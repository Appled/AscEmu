/*
Copyright (c) 2014-2017 AscEmu Team <http://www.ascemu.org/>
This file is released under the MIT license. See README-MIT for more information.
*/

#ifdef USE_EXPERIMENTAL_SPELL_SYSTEM
#include "Spell.h"
#include "SpellHelpers.h"
#include "Map\Area\AreaManagementGlobals.hpp"

using ascemu::World::Spell::Helpers::spellModFlatIntValue;
using ascemu::World::Spell::Helpers::spellModPercentageIntValue;
using ascemu::World::Spell::Helpers::spellModFlatFloatValue;
using ascemu::World::Spell::Helpers::spellModPercentageFloatValue;

bool canAttackCreatureType(uint32_t TargetTypeMask, uint32_t type)
{
    uint32_t cmask = 1 << (type - 1);

    if (type != 0 &&
        TargetTypeMask != 0 &&
        ((TargetTypeMask & cmask) == 0))
        return false;
    else
        return true;
}

Spell::Spell(Object* Caster, SpellInfo const* info, bool triggered, Aura* aur)
{
    ARCEMU_ASSERT(Caster != nullptr && info != nullptr);

    Caster->m_pendingSpells.insert(this);
    m_overrideBasePoints = false;
    m_overridenBasePoints[0] = 0xFFFFFFFF;
    m_overridenBasePoints[1] = 0xFFFFFFFF;
    m_overridenBasePoints[2] = 0xFFFFFFFF;
    chaindamage = 0;
    bDurSet = 0;
    damage = 0;
    bRadSet[0] = 0;
    bRadSet[1] = 0;
    bRadSet[2] = 0;

    if (info->SpellDifficultyID != 0 && Caster->GetTypeId() != TYPEID_PLAYER && Caster->GetMapMgr() && Caster->GetMapMgr()->pInstance)
    {
        if (SpellInfo const* SpellDiffEntry = sSpellFactoryMgr.GetSpellEntryByDifficulty(info->SpellDifficultyID, Caster->GetMapMgr()->iInstanceMode))
            m_spellInfo = SpellDiffEntry;
        else
            m_spellInfo = info;
    }
    else
        m_spellInfo = info;

    m_caster = Caster;
    duelSpell = false;
    m_DelayStep = 0;

    switch (Caster->GetTypeId())
    {
    case TYPEID_PLAYER:
    {
        g_caster = nullptr;
        i_caster = nullptr;
        u_caster = static_cast<Unit*>(Caster);
        p_caster = static_cast<Player*>(Caster);
    }
    break;
    case TYPEID_UNIT:
    {
        g_caster = nullptr;
        i_caster = nullptr;
        p_caster = nullptr;
        u_caster = static_cast<Unit*>(Caster);
    }
    break;
    case TYPEID_ITEM:
    case TYPEID_CONTAINER:
    {
        g_caster = nullptr;
        u_caster = nullptr;
        p_caster = nullptr;
        i_caster = static_cast<Item*>(Caster);
    }
    break;
    case TYPEID_GAMEOBJECT:
    {
        u_caster = nullptr;
        p_caster = nullptr;
        i_caster = nullptr;
        g_caster = static_cast<GameObject*>(Caster);
    }
    break;
    default:
        LogDebugFlag(LF_SPELL, "[DEBUG][SPELL] Incompatible object type, please report this to the dev's");
        break;
    }
    if (u_caster && GetSpellInfo()->AttributesExF & ATTRIBUTESEXF_CAST_BY_CHARMER)
    {
        if (Unit* u = u_caster->GetMapMgrUnit(u_caster->GetCharmedByGUID()))
        {
            u_caster = u;
            if (u->IsPlayer())
                p_caster = static_cast<Player*>(u);
        }
    }

    m_spellState = SPELL_STATE_NULL;

    m_castPositionX = m_castPositionY = m_castPositionZ = 0;
    if (GetSpellInfo()->AttributesExD & ATTRIBUTESEXD_TRIGGERED)
        triggered = true;
    m_triggeredSpell = triggered;
    m_AreaAura = false;

    m_triggeredByAura = aur;

    damageToHit = 0;
    castedItemId = 0;

    m_Spell_Failed = false;
    m_CanRelect = false;
    m_IsReflected = false;
    hadEffect = false;
    bDurSet = false;
    bRadSet[0] = false;
    bRadSet[1] = false;
    bRadSet[2] = false;

    cancastresult = SPELL_CANCAST_OK;

    m_requiresCP = false;
    unitTarget = nullptr;
    itemTarget = nullptr;
    gameObjTarget = nullptr;
    playerTarget = nullptr;
    corpseTarget = nullptr;
    targetConstraintCreature = nullptr;
    targetConstraintGameObject = nullptr;
    add_damage = 0;
    m_Delayed = false;
    pSpellId = 0;
    m_cancelled = false;
    ProcedOnSpell = 0;
    forced_basepoints[0] = forced_basepoints[1] = forced_basepoints[2] = 0;
    extra_cast_number = 0;
    m_reflectedParent = nullptr;
    m_isCasting = false;
    m_glyphslot = 0;
    m_charges = info->procCharges;

    UniqueTargets.clear();
    ModeratedTargets.clear();
    for (uint8 i = 0; i < 3; ++i)
    {
        m_targetUnits[i].clear();
    }

    //create rune avail snapshot
    if (p_caster && p_caster->IsDeathKnight())
        m_rune_avail_before = static_cast<DeathKnight*>(p_caster)->GetRuneFlags();
    else
        m_rune_avail_before = 0;

    m_target_constraint = objmgr.GetSpellTargetConstraintForSpell(info->Id);

    m_missilePitch = 0;
    m_missileTravelTime = 0;
    m_IsCastedOnSelf = false;
    m_castTime = 0;
    m_timer = 0;
    m_magnetTarget = 0;
    Dur = 0;
    m_extraError = SPELL_EXTRA_ERROR_NONE;
}

void Spell::prepare(SpellCastTargets* targets)
{
    if (!m_caster->IsInWorld())
    {
        LogDebugFlag(LF_SPELL, "Object " I64FMT " is casting Spell ID %u while not in World", m_caster->GetGUID(), GetSpellInfo()->Id);
        SendCastResult(SPELL_FAILED_DONT_REPORT);
        finish(false);
        return;
    }

    // In case spell got cast from a script check fear/wander states
    // TODO: this should be handled in Creature AI handler?
    if (!p_caster && u_caster && u_caster->GetAIInterface())
    {
        AIInterface* ai = u_caster->GetAIInterface();
        if (ai->getAiState() == AI_STATE_FEAR || ai->getAiState() == AI_STATE_WANDER)
        {
            SendCastResult(SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW);
            finish(false);
            return;
        }
    }

    m_targets = *targets;
    m_spellState = SPELL_STATE_PREPARING;

    // Prevent casting if we're casting a spell already
    if (!m_triggeredSpell &&
        (u_caster && u_caster->isCastingNonMeleeSpell(false, true, true)) && extra_cast_number)
    {
        SendCastResult(SPELL_FAILED_SPELL_IN_PROGRESS);
        finish(false);
        return;
    }

    // If spell is disabled, do not allow players cast it
    // Game Master flag enabled you can cast disabled spells
    if (objmgr.IsSpellDisabled(GetSpellInfo()->Id) && p_caster && !p_caster->HasFlag(PLAYER_FLAGS, PLAYER_FLAG_GM))
    {
        if (m_triggeredSpell)
            SendCastResult(SPELL_FAILED_DONT_REPORT);
        else
            SendCastResult(SPELL_FAILED_SPELL_UNAVAILABLE);
        finish(false);
        return;
    }

    // Calculate power cost
    m_powerCost = i_caster ? 0 : calculatePowerCost();

    // Does spell require combo points?
    // TODO: also must check is caster controlling vehicle, npc, or another player. if yes, then require CPs
    if ((GetSpellInfo()->hasAttributes(ATTRIBUTESEX_REQ_COMBO_POINTS1) || GetSpellInfo()->hasAttributes(ATTRIBUTESEX_REQ_COMBO_POINTS2)) && !i_caster)
        m_requiresCP = true;

    // Checking can spell be casted
    SpellCastResult rslt = CanCast(false);

    /*chaindamage = 0;
    m_targets = *targets;

    if (!m_triggeredSpell && p_caster != NULL && p_caster->CastTimeCheat)
    m_castTime = 0;
    else
    {
    m_castTime = GetCastTime(sSpellCastTimesStore.LookupEntry(GetSpellInfo()->CastingTimeIndex));

    if (m_castTime && u_caster != nullptr)
    {
    spellModFlatIntValue(u_caster->SM_FCastTime, (int32*)&m_castTime, GetSpellInfo()->SpellGroupType);
    spellModPercentageIntValue(u_caster->SM_PCastTime, (int32*)&m_castTime, GetSpellInfo()->SpellGroupType);
    }

    // handle MOD_CAST_TIME
    if (u_caster != NULL && m_castTime)
    {
    m_castTime = float2int32(m_castTime * u_caster->GetCastSpeedMod());
    }
    }

    if (p_caster != NULL)
    {
    // HookInterface events
    if (!sHookInterface.OnCastSpell(p_caster, GetSpellInfo(), this))
    {
    DecRef();
    return SPELL_FAILED_UNKNOWN;
    }

    if (p_caster->cannibalize)
    {
    sEventMgr.RemoveEvents(p_caster, EVENT_CANNIBALIZE);
    p_caster->SetEmoteState(EMOTE_ONESHOT_NONE);
    p_caster->cannibalize = false;
    }
    }

    //let us make sure cast_time is within decent range
    //this is a hax but there is no spell that has more then 10 minutes cast time

    if (m_castTime < 0)
    m_castTime = 0;
    else if (m_castTime > 60 * 10 * 1000)
    m_castTime = 60 * 10 * 1000; //we should limit cast time to 10 minutes right ?

    m_timer = m_castTime;

    m_magnetTarget = 0;

    //if (p_caster != NULL)
    //   m_castTime -= 100;	  // session update time


    m_spellState = SPELL_STATE_PREPARING;

    if (objmgr.IsSpellDisabled(GetSpellInfo()->Id))//if it's disabled it will not be casted, even if it's triggered.
    cancastresult = uint8(m_triggeredSpell ? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_SPELL_UNAVAILABLE);
    else if (m_triggeredSpell || ProcedOnSpell != NULL)
    cancastresult = SPELL_CANCAST_OK;
    else
    cancastresult = CanCast(false);

    LogDebugFlag(LF_SPELL, "CanCast result: %u. Refer to SpellFailure.h to work out why." , cancastresult);

    ccr = cancastresult;
    if (cancastresult != SPELL_CANCAST_OK)
    {
    SendCastResult(cancastresult);

    if (m_triggeredByAura)
    {
    SendChannelUpdate(0);
    if (u_caster != NULL)
    u_caster->RemoveAura(m_triggeredByAura);
    }
    else
    {
    // HACK, real problem is the way spells are handled
    // when a spell is channeling and a new spell is cast
    // that is a channeling spell, but not triggered by a aura
    // the channel bar/spell is bugged
    if (u_caster && u_caster->GetChannelSpellTargetGUID() != 0 && u_caster->GetCurrentSpell())
    {
    u_caster->GetCurrentSpell()->cancel();
    SendChannelUpdate(0);
    cancel();
    return ccr;
    }
    }
    finish(false);
    return ccr;
    }
    else
    {
    if (p_caster != NULL && p_caster->isStealthed() && !hasAttributes(ATTRIBUTESEX_NOT_BREAK_STEALTH) && GetSpellInfo()->Id != 1)   // <-- baaaad, baaad hackfix - for some reason some spells were triggering Spell ID #1 and stuffing up the spell system.
    {
    // talents procing - don't remove stealth either
    if (!hasAttributes(ATTRIBUTES_PASSIVE) &&
    !(pSpellId && sSpellCustomizations.GetSpellInfo(pSpellId)->IsPassive()))
    {
    p_caster->RemoveAura(p_caster->m_stealth);
    p_caster->m_stealth = 0;
    }
    }

    SendSpellStart();

    // start cooldown handler
    if (p_caster != NULL && !p_caster->CastTimeCheat && !m_triggeredSpell)
    {
    AddStartCooldown();
    }

    if (i_caster == NULL)
    {
    if (p_caster != NULL && m_timer > 0 && !m_triggeredSpell)
    p_caster->delayAttackTimer(m_timer + 1000);
    //p_caster->setAttackTimer(m_timer + 1000, false);
    }

    // aura state removal
    if (GetSpellInfo() && GetSpellInfo()->CasterAuraState != AURASTATE_NONE && GetSpellInfo()->CasterAuraState != AURASTATE_FLAG_JUDGEMENT)
    if (u_caster != nullptr)
    u_caster->RemoveFlag(UNIT_FIELD_AURASTATE, GetSpellInfo()->CasterAuraState);
    }

    //instant cast(or triggered) and not channeling
    if (u_caster != NULL && (m_castTime > 0 || GetSpellInfo()->ChannelInterruptFlags) && !m_triggeredSpell)
    {
    m_castPositionX = m_caster->GetPositionX();
    m_castPositionY = m_caster->GetPositionY();
    m_castPositionZ = m_caster->GetPositionZ();

    u_caster->castSpell(this);
    }
    else
    cast(false);

    return ccr;*/
}

uint8_t Spell::getSpellDamageType() const
{
    uint8_t result = 0;
    switch (GetSpellInfo()->Spell_Dmg_Type)
    {
    case SPELL_DMG_TYPE_MELEE:
        if (GetSpellInfo()->hasAttributes(ATTRIBUTESEXC_TYPE_OFFHAND))
            result = OFFHAND;
        else
            result = MELEE;
        break;
    case SPELL_DMG_TYPE_RANGED:
    {
        const int32_t itemSubClassRangedWeaponsMask = ((1 << ITEM_SUBCLASS_WEAPON_BOW) | (1 << ITEM_SUBCLASS_WEAPON_GUN) | (1 << ITEM_SUBCLASS_WEAPON_CROSSBOW) | (1 << ITEM_SUBCLASS_WEAPON_THROWN));
        // Hunter's Explosive Shot is a dirty case
        if ((GetSpellInfo()->SpellFamilyName == 9 && !(GetSpellInfo()->SpellGroupType[1] & 0x10000000)) || (GetSpellInfo()->EquippedItemSubClass & itemSubClassRangedWeaponsMask))
            result = RANGED;
        else
            result = MELEE;
        break;
    }
    default:
        // Wands
        if (GetSpellInfo()->hasAttributes(ATTRIBUTESEXB_AUTO_REPEAT))
            result = RANGED;
        else
            result = MELEE;
        break;
    }
    return result;
}

int32_t Spell::calculatePowerCost() const
{
    if (GetSpellInfo()->hasAttributes(ATTRIBUTESEX_DRAIN_WHOLE_POWER) && u_caster)
    {
        if (GetSpellInfo()->powerType == POWER_TYPE_HEALTH)
            return int32_t(u_caster->GetHealth());
        else if (GetSpellInfo()->powerType <= POWER_TYPE_RUNIC_POWER)
            return int32_t(u_caster->GetPower(GetSpellInfo()->powerType));
    }

    int32_t powerCost = GetSpellInfo()->manaCost;
    if (u_caster)
    {
        // If spell cost is percentage of caster's power
        if (GetSpellInfo()->ManaCostPercentage)
        {
            switch (GetSpellInfo()->powerType)
            {
            case POWER_TYPE_HEALTH:
                powerCost += int32_t(u_caster->GetBaseHealth() * GetSpellInfo()->ManaCostPercentage / 100);
                break;
            case POWER_TYPE_MANA:
                powerCost += int32_t(u_caster->GetBaseMana() * GetSpellInfo()->ManaCostPercentage / 100);
                break;
            case POWER_TYPE_RAGE:
            case POWER_TYPE_FOCUS:
            case POWER_TYPE_ENERGY:
            case POWER_TYPE_HAPPINESS:
                powerCost += int32_t(u_caster->GetMaxPower(GetSpellInfo()->powerType) * GetSpellInfo()->ManaCostPercentage / 100);
                break;
            case POWER_TYPE_RUNES:
            case POWER_TYPE_RUNIC_POWER:
                // In 3.3.5a only test spells have these and DK's old Summon Gargoyle spell from pre-3.2
                break;
            default:
                LOG_ERROR("Spell::calculatePowerCost: Unknown power type %d for spell id %d", GetSpellInfo()->powerType, GetSpellInfo()->Id);
                return 0;
            }
        }
        // Use first school in mask
        uint32_t spellSchool = SCHOOL_NORMAL;
        for (int i = 0; i < SCHOOL_COUNT; ++i)
        {
            if (GetSpellInfo()->School & (1 << i))
            {
                spellSchool = i;
                break;
            }
        }
        // Include power cost modifiers from same spell school
        powerCost += u_caster->getUInt32Value(UNIT_FIELD_POWER_COST_MODIFIER + spellSchool);

        // Special case for rogue's Shiv
        if (GetSpellInfo()->hasAttributes(ATTRIBUTESEXD_UNK7))
        {
            uint32_t speed = 0;
            if (auto shapeshiftForm = sSpellShapeshiftFormStore.LookupEntry(u_caster->GetShapeShift()))
                speed = shapeshiftForm->AttackSpeed;
            else
            {
                if (p_caster)
                {
                    int16_t slot = 0;
                    switch (getSpellDamageType())
                    {
                    case MELEE:
                        slot = EQUIPMENT_SLOT_MAINHAND;
                        break;
                    case OFFHAND:
                        slot = EQUIPMENT_SLOT_OFFHAND;
                        break;
                    case RANGED:
                        slot = EQUIPMENT_SLOT_RANGED;
                        break;
                    default:
                        break;
                    }
                    Item* it = p_caster->GetItemInterface()->GetInventoryItem(slot);
                    if (slot != 0 && it)
                        speed = it->GetItemProperties()->Delay;
                    else
                        speed = p_caster->GetBaseAttackTime(getSpellDamageType());
                }
                else
                    speed = u_caster->GetBaseAttackTime(getSpellDamageType());
            }
            powerCost += speed / 100;
        }

        // Apply modifiers
        spellModFlatIntValue(u_caster->SM_FCost, &powerCost, GetSpellInfo()->SpellGroupType);
        spellModPercentageIntValue(u_caster->SM_PCost, &powerCost, GetSpellInfo()->SpellGroupType);

        // Include percent modifier from same spell school
        powerCost = int32_t(powerCost * (1.0f + u_caster->getFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER + spellSchool)));
    }

    if (powerCost < 0)
        powerCost = 0;
    return powerCost;
}

SpellCastResult Spell::checkRange(bool tolerate) const
{
    auto rangeEntry = sSpellRangeStore.LookupEntry(GetSpellInfo()->rangeIndex);
    if (!m_triggeredSpell && u_caster && GetSpellInfo()->rangeIndex)
    {
        Unit* unitTarget = m_caster->GetMapMgrUnit(m_targets.m_unitTarget);
        float minRange, maxRange, rangeMod = 0.0f;
        if (tolerate && GetSpellInfo()->hasAttributes(SpellAttributes(ATTRIBUTES_ON_NEXT_ATTACK | ATTRIBUTES_ON_NEXT_SWING_2)))
            maxRange = 100.0f;

        if (rangeEntry)
        {
            if (rangeEntry->range_type & 1) // Melee range
                                            // If target is not unit, use caster's combat reach as target's combat reach and sum them. If smaller than 5 yards, use default 5 yards.
                rangeMod = std::max((u_caster->GetCombatReach() + (unitTarget ? unitTarget->GetCombatReach() : u_caster->GetCombatReach()) + 4.0f / 3.0f), 5.0f);
            else
            {
                float meleeRange = 0.0f;
                if (rangeEntry->range_type & 2) // Ranged spells
                                                // If target is not unit, use caster's combat reach as target's combat reach and sum them. If smaller than 5 yards, use default 5 yards.
                    meleeRange = std::max((u_caster->GetCombatReach() + (unitTarget ? unitTarget->GetCombatReach() : u_caster->GetCombatReach()) + 4.0f / 3.0f), 5.0f);

                // Get minimum range for spell
                if (rangeEntry->minRangeFriendly == rangeEntry->minRange)
                    minRange = rangeEntry->minRange;
                else if (!unitTarget)
                    minRange = rangeEntry->minRangeFriendly;
                else
                    minRange = isFriendly(u_caster, unitTarget) ? rangeEntry->minRangeFriendly : rangeEntry->minRange;
                minRange += meleeRange;

                // Get maximum range for spell
                if (rangeEntry->maxRangeFriendly == rangeEntry->maxRange)
                    maxRange = rangeEntry->maxRange;
                else if (!unitTarget)
                    maxRange = rangeEntry->maxRangeFriendly;
                else
                    maxRange = isFriendly(u_caster, unitTarget) ? rangeEntry->maxRangeFriendly : rangeEntry->maxRange;

                if (unitTarget || m_targets.m_targetMask & (TARGET_FLAG_CORPSE | TARGET_FLAG_CORPSE2))
                {
                    rangeMod = u_caster->GetCombatReach() + (unitTarget ? unitTarget->GetCombatReach() : u_caster->GetCombatReach());
                    if (minRange > 0.0f && !(rangeEntry->range_type & 2))
                        minRange += rangeMod;
                }
            }

            // Spell leeway - client increases the spell's range if you are moving (walking is not counted)
            if ((u_caster->HasUnitMovementFlag(MOVEFLAG_MOVE_FORWARD) || u_caster->HasUnitMovementFlag(MOVEFLAG_MOVE_BACKWARD) || u_caster->HasUnitMovementFlag(MOVEFLAG_STRAFE_LEFT) ||
                u_caster->HasUnitMovementFlag(MOVEFLAG_STRAFE_RIGHT) || u_caster->HasUnitMovementFlag(MOVEFLAG_FALLING) | u_caster->HasUnitMovementFlag(MOVEFLAG_FALLING_FAR) ||
                u_caster->HasUnitMovementFlag(MOVEFLAG_ASCENDING) || u_caster->HasUnitMovementFlag(MOVEFLAG_CAN_FLY) || u_caster->HasUnitMovementFlag(MOVEFLAG_SPLINE_MOVER)) &&
                !u_caster->HasUnitMovementFlag(MOVEFLAG_WALK))
            {
                // Spell leeway also depends on target - if target is moving (again walking is not counted) => increase spell range by 2.66 yards
                if (unitTarget)
                {
                    if ((unitTarget->HasUnitMovementFlag(MOVEFLAG_MOVE_FORWARD) || unitTarget->HasUnitMovementFlag(MOVEFLAG_MOVE_BACKWARD) || unitTarget->HasUnitMovementFlag(MOVEFLAG_STRAFE_LEFT) ||
                        unitTarget->HasUnitMovementFlag(MOVEFLAG_STRAFE_RIGHT) || unitTarget->HasUnitMovementFlag(MOVEFLAG_FALLING) | unitTarget->HasUnitMovementFlag(MOVEFLAG_FALLING_FAR) ||
                        unitTarget->HasUnitMovementFlag(MOVEFLAG_DESCENDING) || unitTarget->HasUnitMovementFlag(MOVEFLAG_CAN_FLY) || unitTarget->HasUnitMovementFlag(MOVEFLAG_SPLINE_MOVER)) &&
                        !unitTarget->HasUnitMovementFlag(MOVEFLAG_WALK))
                        rangeMod += 8.0f / 3.0f; // 2.66666... yards

                }
            }
        }

        // Include range from ranged weapon if spell is to be casted on next ranged attack
        if (GetSpellInfo()->hasAttributes(ATTRIBUTES_RANGED) && p_caster)
        {
            if (Item* rangedWeapon = p_caster->GetItemInterface()->GetInventoryItem(EQUIPMENT_SLOT_RANGED))
                maxRange *= rangedWeapon->GetItemProperties()->Range * 0.01f;
        }

        // Apply modifiers
        spellModFlatFloatValue(u_caster->SM_FRange, &maxRange, GetSpellInfo()->SpellGroupType);
        spellModPercentageFloatValue(u_caster->SM_PRadius, &maxRange, GetSpellInfo()->SpellGroupType);

        maxRange += rangeMod;
        if (rangeEntry && rangeEntry->range_type != 1 && !tolerate)
            maxRange += std::min(3.0f, maxRange*0.1f);

        // Square values for distance checks
        minRange *= minRange;
        maxRange *= maxRange;

        if (unitTarget && unitTarget != u_caster)
        {
            if (u_caster->GetDistance2dSq(unitTarget) > maxRange)
                return SPELL_FAILED_OUT_OF_RANGE;
            if (minRange > 0.0f && u_caster->GetDistance2dSq(unitTarget) < minRange)
                return SPELL_FAILED_OUT_OF_RANGE;
            if ((GetSpellInfo()->FacingCasterFlags & 1) && !u_caster->isInFront(unitTarget))
                return SPELL_FAILED_UNIT_NOT_INFRONT;
        }

        // AoE spells on targeted location
        if (m_targets.hasDestination())
        {
            if (u_caster->getDistanceSq(m_targets.destination()) > maxRange)
                return SPELL_FAILED_OUT_OF_RANGE;
            if (minRange > 0.0f && u_caster->getDistanceSq(m_targets.destination()) < minRange)
                return SPELL_FAILED_OUT_OF_RANGE;
        }
    }
    return SPELL_CANCAST_OK;
}

SpellCastResult Spell::hasRunes() const
{
    // Only handle spells with rune cost
    if (!GetSpellInfo()->RuneCostID)
        return SPELL_CANCAST_OK;

    // Handle only player cases
    if (!p_caster)
        return SPELL_CANCAST_OK;

    // and death knight cases
    if (!p_caster->getClass() != DEATHKNIGHT)
        return SPELL_CANCAST_OK;

    auto spellRuneCost = sSpellRuneCostStore.LookupEntry(GetSpellInfo()->RuneCostID);
    if (!spellRuneCost)
        return SPELL_CANCAST_OK;

    // If spell does not cost runes
    if (spellRuneCost->bloodRuneCost == 0 && spellRuneCost->frostRuneCost == 0 && spellRuneCost->unholyRuneCost == 0)
        return SPELL_CANCAST_OK;

    int32_t runeCost[3];
    runeCost[RUNE_BLOOD] = int32_t(spellRuneCost->bloodRuneCost);
    runeCost[RUNE_FROST] = int32_t(spellRuneCost->frostRuneCost);
    runeCost[RUNE_UNHOLY] = int32_t(spellRuneCost->unholyRuneCost);

    // Apply spell cost modifiers
    for (int i = 0; i < RUNE_DEATH; ++i)
    {
        spellModFlatIntValue(p_caster->SM_FCost, &runeCost[i], GetSpellInfo()->SpellGroupType);
        spellModPercentageIntValue(p_caster->SM_PCost, &runeCost[i], GetSpellInfo()->SpellGroupType);
    }

    // Calculate the missing runes by subtracting all available runes from power cost
    DeathKnight* dkPlayer = static_cast<DeathKnight*>(p_caster);
    const uint32_t missingRunes = dkPlayer->HasRunes(RUNE_BLOOD, runeCost[RUNE_BLOOD]) +
        dkPlayer->HasRunes(RUNE_FROST, runeCost[RUNE_FROST]) +
        dkPlayer->HasRunes(RUNE_UNHOLY, runeCost[RUNE_UNHOLY]);

    // In case if all normal runes are on cooldown, check for available death runes
    if (missingRunes > 0 && dkPlayer->HasRunes(RUNE_DEATH, missingRunes) > 0)
        return SPELL_FAILED_NO_POWER;

    return SPELL_CANCAST_OK;
}

SpellCastResult Spell::HasPower() const
{
    // Items do not use owner's power
    if (i_caster)
        return SPELL_CANCAST_OK;

    if (p_caster && p_caster->PowerCheat)
        return SPELL_CANCAST_OK;

    // Health is used as power
    if (GetSpellInfo()->powerType == POWER_TYPE_HEALTH)
    {
        if (u_caster && int32_t(u_caster->GetHealth()) <= m_powerCost)
            return SPELL_FAILED_CASTER_AURASTATE;
        return SPELL_CANCAST_OK;
    }

    // Invalid power types
    if (GetSpellInfo()->powerType > POWER_TYPE_RUNIC_POWER)
    {
        LOG_ERROR("Spell::hasPower: Unknown power type %d for spell id %d", GetSpellInfo()->powerType, GetSpellInfo()->Id);
        return SPELL_FAILED_UNKNOWN;
    }

    // Runes are used as power
    if (GetSpellInfo()->powerType == POWER_TYPE_RUNES)
    {
        SpellCastResult runeCheck = hasRunes();
        if (runeCheck != SPELL_CANCAST_OK)
            return runeCheck;
    }

    // Normal cases
    if (u_caster && u_caster->GetPower(GetSpellInfo()->powerType) <= m_powerCost)
        return SPELL_FAILED_NO_POWER;

    return SPELL_CANCAST_OK;
}

SpellCastResult Spell::checkCasterState() const
{
    if (GetSpellInfo()->hasAttributes(ATTRIBUTESEXG_IGNORE_CASTER_AURAS))
        return SPELL_CANCAST_OK;

    // Check if spell gives school, mechanic or dispel immunity
    // Use bitmask to run loop only once
    // from MaNGOS
    uint8_t schoolImmunity = 0;
    uint32_t mechanicImmunity = 0;
    uint32_t dispelImmunity = 0;

    if (GetSpellInfo()->hasAttributes(ATTRIBUTESEX_DISPEL_AURAS_ON_IMMUNITY))
    {
        for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            switch (GetSpellInfo()->EffectApplyAuraName[i])
            {
            case SPELL_AURA_SCHOOL_IMMUNITY:
                schoolImmunity |= uint32_t(GetSpellInfo()->EffectMiscValue[i]);
                break;
            case SPELL_AURA_MECHANIC_IMMUNITY:
                mechanicImmunity |= 1 << uint32_t(GetSpellInfo()->EffectMiscValue[i] - 1);
                break;
            case SPELL_AURA_MECHANIC_IMMUNITY_MASK:
                mechanicImmunity |= uint32_t(GetSpellInfo()->EffectMiscValue[i]);
                break;
            case SPELL_AURA_DISPEL_IMMUNITY:
            {
                const uint32_t dispelMaskAll = ((1 << DISPEL_MAGIC) | (1 << DISPEL_CURSE) | (1 << DISPEL_DISEASE) | (1 << DISPEL_POISON));
                dispelImmunity |= (GetSpellInfo()->EffectMiscValue[i] != DISPEL_ALL ? uint32_t(1 << GetSpellInfo()->EffectMiscValue[i]) : dispelMaskAll);
            } break;
            default:
                break;
            }
        }

        // Check if spell removes all movement impairment and loss of control effects
        if (GetSpellInfo()->EffectApplyAuraName[0] == SPELL_AURA_MECHANIC_IMMUNITY &&
            GetSpellInfo()->EffectMiscValue[0] == 1 &&
            GetSpellInfo()->EffectApplyAuraName[1] == 0 &&
            GetSpellInfo()->EffectApplyAuraName[2] == 0 &&
            GetSpellInfo()->hasAttributes(ATTRIBUTESEX_DISPEL_AURAS_ON_IMMUNITY))
            mechanicImmunity = MOVEMENT_IMPAIRMENTS_AND_LOSS_OF_CONTROL_MASK;
    }

    SpellCastResult errorMsg = SPELL_CANCAST_OK;
    const uint32_t unitState = m_caster->getUInt32Value(UNIT_FIELD_FLAGS);
    if (unitState & UNIT_FLAG_STUNNED)
    {
        // We must check is there any other stun auras without stun mechanic
        if (GetSpellInfo()->hasAttributes(ATTRIBUTESEXE_USABLE_WHILE_STUNNED) && u_caster)
        {
            bool isStunMechanic = true;
            for (uint32_t i = MAX_TOTAL_AURAS_START; i < MAX_TOTAL_AURAS_END; ++i)
            {
                if (u_caster->m_auras[i] == nullptr)
                    continue;
                if (!u_caster->m_auras[i]->HasModType(SPELL_AURA_MOD_STUN))
                    continue;
                if (u_caster->m_auras[i]->GetSpellInfo()->MechanicsType != MECHANIC_STUNNED)
                {
                    for (int x = 0; x < MAX_SPELL_EFFECTS; ++x)
                    {
                        if (u_caster->m_auras[i]->GetSpellInfo()->EffectMechanic[x] != MECHANIC_STUNNED)
                        {
                            isStunMechanic = false;
                            break;
                        }
                    }
                    // if boolean is set to false in inner loop, we don't need to check further
                    if (!isStunMechanic)
                        break;
                }
            }

            if (!isStunMechanic)
                errorMsg = SPELL_FAILED_STUNNED;
        }
        else
            errorMsg = SPELL_FAILED_STUNNED;
    }
    else if (unitState & UNIT_FLAG_CONFUSED && !GetSpellInfo()->hasAttributes(ATTRIBUTESEXE_USABLE_WHILE_CONFUSED))
        errorMsg = SPELL_FAILED_CONFUSED;
    else if (unitState & UNIT_FLAG_FLEEING && !GetSpellInfo()->hasAttributes(ATTRIBUTESEXE_USABLE_WHILE_FEARED))
        errorMsg = SPELL_FAILED_FLEEING;
    else if (unitState & UNIT_FLAG_SILENCED && GetSpellInfo()->PreventionType == 1)
        errorMsg = SPELL_FAILED_SILENCED;
    else if (unitState & UNIT_FLAG_PACIFIED && GetSpellInfo()->PreventionType == 2)
        errorMsg = SPELL_FAILED_PACIFIED;
    else if (u_caster && u_caster->hasAuraWithAuraType(SPELL_AURA_ALLOW_ONLY_ABILITY))
    {
        for (uint32_t i = MAX_TOTAL_AURAS_START; i < MAX_TOTAL_AURAS_END; ++i)
        {
            if (u_caster->m_auras[i] == nullptr)
                continue;
            if (!u_caster->m_auras[i]->HasModType(SPELL_AURA_ALLOW_ONLY_ABILITY))
                continue;

            SpellInfo const* auraSpellInfo = u_caster->m_auras[i]->GetSpellInfo();
            if (!auraSpellInfo)
                continue;

            if (!auraSpellInfo->isAffectedBySpell(GetSpellInfo()))
            {
                errorMsg = SPELL_FAILED_CASTER_AURASTATE;
                break;
            }
        }
    }

    if (errorMsg != SPELL_CANCAST_OK)
    {
        if ((schoolImmunity || mechanicImmunity || dispelImmunity) && u_caster)
        {
            // You are prevented to cast the spell by some state but the spell you are casting grants immunity
            for (uint32_t i = MAX_TOTAL_AURAS_START; i < MAX_TOTAL_AURAS_END; ++i)
            {
                Aura* auraSpell = u_caster->m_auras[i];
                if (auraSpell == nullptr)
                    continue;
                SpellInfo const* auraProto = auraSpell->GetSpellInfo();
                if (auraProto == nullptr)
                    continue;
                if ((auraProto->School & schoolImmunity) && !auraProto->hasAttributes(ATTRIBUTESEX_UNAFFECTED_BY_SCHOOL_IMMUNE))
                    continue;
                if ((1 << (auraProto->DispelType)) & dispelImmunity)
                    continue;

                uint32_t mechanicMask = 0;
                if (auraProto->MechanicsType)
                    mechanicMask |= 1 << (auraProto->MechanicsType - 1);

                for (int x = 0; x < MAX_SPELL_EFFECTS; ++x)
                {
                    if (!auraProto->Effect[x])
                        continue;
                    if (auraProto->EffectMechanic[x])
                        mechanicMask |= uint32_t(1 << (auraProto->EffectMechanic[x] - 1));
                }

                if (mechanicMask & mechanicImmunity)
                    continue;

                // Your casting is prevented by multiple states but you are only immune to some of them
                // so return correct error message
                if (auraSpell->HasModType(SPELL_AURA_MOD_STUN))
                {
                    if (!GetSpellInfo()->hasAttributes(ATTRIBUTESEXE_USABLE_WHILE_STUNNED))
                        return SPELL_FAILED_STUNNED;
                    bool hasStunMechanic = false;
                    for (int x = 0; x < MAX_SPELL_EFFECTS; ++x)
                    {
                        if (!auraProto->Effect[x])
                            continue;
                        if (auraProto->EffectMechanic[x] == MECHANIC_STUNNED)
                        {
                            hasStunMechanic = true;
                            break;
                        }
                    }
                    if (!hasStunMechanic)
                        return SPELL_FAILED_STUNNED;
                }
                else if (auraSpell->HasModType(SPELL_AURA_MOD_CONFUSE))
                {
                    if (!GetSpellInfo()->hasAttributes(ATTRIBUTESEXE_USABLE_WHILE_CONFUSED))
                        return SPELL_FAILED_CONFUSED;
                }
                else if (auraSpell->HasModType(SPELL_AURA_MOD_FEAR))
                {
                    if (!GetSpellInfo()->hasAttributes(ATTRIBUTESEXE_USABLE_WHILE_FEARED))
                        return SPELL_FAILED_FLEEING;
                }
                else if (auraSpell->HasModType(SPELL_AURA_MOD_SILENCE) || auraSpell->HasModType(SPELL_AURA_MOD_PACIFY) || auraSpell->HasModType(SPELL_AURA_MOD_PACIFY_SILENCE))
                {
                    if (GetSpellInfo()->PreventionType == 2)
                        return SPELL_FAILED_PACIFIED;
                    else if (GetSpellInfo()->PreventionType == 1)
                        return SPELL_FAILED_SILENCED;
                }
            }
        }
        // State prevents from casting and spell does not grant any immunities => return error message
        else
            return errorMsg;
    }
    return SPELL_CANCAST_OK;
}

SpellCastResult Spell::CanCast(bool tolerate, uint32_t* parameter1, uint32_t* parameter2)
{
    if (!GetSpellInfo()->hasAttributes(ATTRIBUTES_PASSIVE) && p_caster)
    {
        // Can't cast other spells if you have SPELL_AURA_ALLOW_ONLY_ABILITY auras (Killing Spree and Bladestorm for example)
        if (!m_triggeredSpell && p_caster->HasFlag(PLAYER_FLAGS, PLAYER_FLAG_ALLOW_ONLY_ABILITY))
            return SPELL_FAILED_SPELL_IN_PROGRESS;

        // Check cooldowns
        if (!m_triggeredSpell && !p_caster->Cooldown_CanCast(GetSpellInfo()))
        {
            if (m_triggeredByAura)
                return SPELL_FAILED_DONT_REPORT;
            else
                return SPELL_FAILED_NOT_READY;
        }
    }

    // Check is caster alive
    if (u_caster && !u_caster->isAlive() &&
        !(GetSpellInfo()->hasAttributes(ATTRIBUTES_DEAD_CASTABLE) || (m_triggeredSpell && !m_triggeredByAura)))
        return SPELL_FAILED_CASTER_DEAD;

    if (p_caster && p_caster->m_bg)
    {
        if (IS_ARENA(p_caster->m_bg->GetType()))
        {
            const uint32_t recoveryTime = GetSpellInfo()->RecoveryTime > GetSpellInfo()->CategoryRecoveryTime ? GetSpellInfo()->RecoveryTime : GetSpellInfo()->CategoryRecoveryTime;
            if (GetSpellInfo()->hasAttributes(ATTRIBUTESEXD_NOT_IN_ARENA) || (recoveryTime > 10 * MINUTE * IN_MILLISECONDS && !GetSpellInfo()->hasAttributes(ATTRIBUTESEXD_USABLE_IN_ARENA)))
                return SPELL_FAILED_NOT_IN_ARENA;
        }
        // If battleground has ended but player is still inside
        // dont allow to cast spells, unless it's a triggered spell
        if (!m_triggeredSpell && p_caster->m_bg->HasEnded())
            return SPELL_FAILED_DONT_REPORT;
    }
    else if (GetSpellInfo()->hasAttributes(ATTRIBUTESEXC_BG_ONLY))
        return SPELL_FAILED_ONLY_BATTLEGROUNDS;

    bool requireCombat = true;
    if (u_caster && u_caster->hasAuraWithAuraType(SPELL_AURA_IGNORE_TARGET_AURA_STATE))
    {
        for (uint32_t u = MAX_TOTAL_AURAS_START; u < MAX_TOTAL_AURAS_END; ++u)
        {
            if (!u_caster->m_auras[u])
                continue;
            if (!u_caster->m_auras[u]->HasModType(SPELL_AURA_IGNORE_TARGET_AURA_STATE))
                continue;
            if (GetSpellInfo()->isAffectedBySpell(u_caster->m_auras[u]->GetSpellInfo()))
            {
                // Some non-rogue class spells have 'requires combo points' attribute set to control special cases
                m_requiresCP = false;
                // Every aura with this type uses effect index 0
                // This handles warrior's Warbringer and Juggernaut talents (can cast Charge while in combat)
                if (u_caster->m_auras[u]->GetSpellInfo()->EffectMiscValue[0] == 1)
                {
                    requireCombat = false;
                    break;
                }
            }
        }
    }

    // Out of combat spells should not be able to be casted in combat
    if (!m_triggeredSpell && requireCombat && GetSpellInfo()->hasAttributes(ATTRIBUTES_REQ_OOC) && u_caster && u_caster->CombatStatus.IsInCombat())
        return SPELL_FAILED_AFFECTING_COMBAT;

    // Only Game Masters can cast spells with cheat attribute
    if (GetSpellInfo()->hasAttributes(ATTRIBUTESEXG_IS_CHEAT_SPELL) && p_caster && !p_caster->HasFlag(PLAYER_FLAGS, PLAYER_FLAG_GM))
    {
        m_extraError = SPELL_EXTRA_ERROR_GM_ONLY;
        return SPELL_FAILED_CUSTOM_ERROR;
    }

    // Indoor and outdoor specific spells
    if (worldConfig.terrainCollision.isCollisionEnabled)
    {
        if (GetSpellInfo()->hasAttributes(ATTRIBUTES_ONLY_OUTDOORS) &&
            !MapManagement::AreaManagement::AreaStorage::IsOutdoor(m_caster->GetMapId(), m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ()))
            return SPELL_FAILED_ONLY_OUTDOORS;

        if (GetSpellInfo()->hasAttributes(ATTRIBUTES_ONLY_INDOORS) &&
            MapManagement::AreaManagement::AreaStorage::IsOutdoor(m_caster->GetMapId(), m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ()))
            return SPELL_FAILED_ONLY_INDOORS;
    }

    // Skipping triggered spells in this first check
    if (tolerate && !m_triggeredSpell && u_caster)
    {
        // If aura has ignore shapeshift type, you can use spells regardless of stance / form
        // Auras with this type: Shadow Dance, Metamorphosis and Warbringer (3.3.5a)
        bool hasIgnoreShapeshiftAura = false;
        for (uint32_t u = MAX_TOTAL_AURAS_START; u < MAX_TOTAL_AURAS_END; ++u)
        {
            if (!u_caster->m_auras[u])
                continue;
            if (!u_caster->m_auras[u]->HasModType(SPELL_AURA_IGNORE_SHAPESHIFT))
                continue;
            if (GetSpellInfo()->isAffectedBySpell(u_caster->m_auras[u]->GetSpellInfo()))
            {
                hasIgnoreShapeshiftAura = true;
                break;
            }
        }
        if (!hasIgnoreShapeshiftAura)
        {
            SpellCastResult shapeError = getErrorAtShapeshiftedCast(GetSpellInfo(), u_caster->GetShapeShift());
            if (shapeError != SPELL_FAILED_SUCCESS)
                return shapeError;

            if (GetSpellInfo()->hasAttributes(ATTRIBUTES_REQ_STEALTH) && !u_caster->hasAuraWithAuraType(SPELL_AURA_MOD_STEALTH))
                return SPELL_FAILED_ONLY_STEALTHED;
        }
    }

    // Caster's aura state requirements
    if (GetSpellInfo()->CasterAuraState && !u_caster->hasAuraState(AuraStates(GetSpellInfo()->CasterAuraState), GetSpellInfo(), u_caster))
        return SPELL_FAILED_CASTER_AURASTATE;
    if (GetSpellInfo()->CasterAuraStateNot && u_caster->hasAuraState(AuraStates(GetSpellInfo()->CasterAuraStateNot), GetSpellInfo(), u_caster))
        return SPELL_FAILED_CASTER_AURASTATE;

    // Caster's aura spell requirements
    if (GetSpellInfo()->casterAuraSpell && !u_caster->HasAura(GetSpellInfo()->casterAuraSpell))
        return SPELL_FAILED_CASTER_AURASTATE;
    if (GetSpellInfo()->casterAuraSpellNot)
    {
        // TODO: MOVE THESE TO SPELL HANDLER LIBRARY
        // I think there are like 5 or 6 spells with this spell in 'casterAuraSpellNot'
        // load script for them all to check for this immunity marker?
        if (GetSpellInfo()->casterAuraSpellNot == 61988)
        {
            if (u_caster->HasAura(61987))
                return SPELL_FAILED_CASTER_AURASTATE;
        }
        else if (u_caster->HasAura(GetSpellInfo()->casterAuraSpellNot))
            return SPELL_FAILED_CASTER_AURASTATE;
    }

    if (p_caster && p_caster->m_isMoving)
    {
        // Cancel autorepeat spells if cast starts when moving
        // skip Unstuck spell, to allow use it in falling case (from MaNGOS)
        if ((!(p_caster->movement_info.flags & MOVEFLAG_FALLING_FAR) || GetSpellInfo()->Effect[0] != SPELL_EFFECT_STUCK) &&
            (GetSpellInfo()->hasAttributes(ATTRIBUTESEXB_AUTO_REPEAT) || (GetSpellInfo()->AuraInterruptFlags & AURA_INTERRUPT_ON_STAND_UP) != 0))
            return SPELL_FAILED_MOVING;
    }

    // Check if spell requires to be in combat to be casted
    if (!u_caster->CombatStatus.IsInCombat() && GetSpellInfo()->hasAttributes(ATTRIBUTES_STOP_ATTACK) && GetSpellInfo()->hasAttributes(ATTRIBUTESEXB_UNK28))
        return SPELL_FAILED_CASTER_AURASTATE;

    // Unit target checks
    Unit* target = m_caster->GetMapMgrUnit(m_targets.m_unitTarget);
    if (target)
    {
        if (GetSpellInfo()->TargetAuraState && !target->hasAuraState(AuraStates(GetSpellInfo()->TargetAuraState), GetSpellInfo(), u_caster))
            return SPELL_FAILED_TARGET_AURASTATE;

        if (GetSpellInfo()->TargetAuraStateNot && target->hasAuraState(AuraStates(GetSpellInfo()->TargetAuraStateNot), GetSpellInfo(), u_caster))
            return SPELL_FAILED_TARGET_AURASTATE;

        if (GetSpellInfo()->targetAuraSpell && !target->HasAura(GetSpellInfo()->targetAuraSpell))
            return SPELL_FAILED_TARGET_AURASTATE;

        if (GetSpellInfo()->targetAuraSpellNot)
        {
            // TODO: MOVE THESE TO SPELL HANDLER LIBRARY
            // I think there are like 5 or 6 spells with this spell in 'targetAuraSpellNot'
            // load script for them all to check for this immunity marker?
            if (GetSpellInfo()->targetAuraSpellNot == 61988)
            {
                if (target->HasAura(61987))
                    return SPELL_FAILED_TARGET_AURASTATE;
            }
            else if (target->HasAura(GetSpellInfo()->targetAuraSpellNot))
                return SPELL_FAILED_TARGET_AURASTATE;
        }

        if (target->IsDead())
        {
            // If target is dead but bare bones are all what's left (or corpse cannot be found), we can't cast any spells on it
            Corpse* unitCorpse = objmgr.GetCorpseByOwner(target->GetLowGUID());
            if (!unitCorpse || !unitCorpse->IsInWorld() || unitCorpse->GetCorpseState() == CORPSE_STATE_BONES)
                return SPELL_FAILED_BAD_TARGETS;
        }

        if (GetSpellInfo()->hasAttributes(ATTRIBUTESEX_CANT_TARGET_SELF) && m_caster == target)
            return SPELL_FAILED_BAD_TARGETS;

        if (!GetSpellInfo()->hasAttributes(ATTRIBUTESEXF_CAN_TARGET_INVISIBLE) && !(u_caster && u_caster->canSeeOrDetect(target, g_caster != nullptr)))
            return SPELL_FAILED_BAD_TARGETS;

        if (GetSpellInfo()->hasAttributes(ATTRIBUTESEX_REQ_OOC_TARGET) && target->CombatStatus.IsInCombat())
            return SPELL_FAILED_TARGET_AFFECTING_COMBAT;

        if (GetSpellInfo()->hasAttributes(ATTRIBUTESEXC_TARGET_ONLY_GHOSTS) != target->hasAuraWithAuraType(SPELL_AURA_GHOST))
        {
            if (GetSpellInfo()->hasAttributes(ATTRIBUTESEXC_TARGET_ONLY_GHOSTS))
                return SPELL_FAILED_TARGET_NOT_GHOST;
            else
                return SPELL_FAILED_BAD_TARGETS;
        }

        if (m_caster != target)
        {
            // Hostile / friendly check
            // TODO: maybe not need to implement...
            /*bool checkedTargetResult = false, targetHostile = false, targetHostileChecked = false, targetFriendly = false, targetFriendlyChecked = false;
            for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
            if (isPositiveSpellEffectTarget(GetSpellInfo()->EffectImplicitTargetA[i]))
            {
            if (!targetHostileChecked)
            {
            targetHostileChecked = true;
            targetHostile = isHostile(m_caster, target);
            }

            if (targetHostile)
            return SPELL_FAILED_BAD_TARGETS;

            checkedTargetResult = true;
            }
            else if (isNegativeSpellEffectTarget(GetSpellInfo()->EffectImplicitTargetA[i]))
            {
            if (!targetFriendlyChecked)
            {
            targetFriendlyChecked = true;
            targetFriendly = isFriendly(m_caster, target);
            }

            if (targetFriendly)
            return SPELL_FAILED_BAD_TARGETS;

            checkedTargetResult = true;
            }
            }

            if (!checkedTargetResult && u_caster && (u_caster->GetUInt64Value(UNIT_FIELD_SUMMONEDBY) != 0 || u_caster->GetUInt64Value(UNIT_FIELD_CHARMEDBY) != 0))
            {
            }*/

            if (p_caster)
            {
                // Check if we can attack this creature type
                // but skip checks for Grounding Totem
                if (target->IsCreature() && target->getUInt32Value(UNIT_CREATED_BY_SPELL) != 8177)
                {
                    if (!canAttackCreatureType(GetSpellInfo()->TargetCreatureType, static_cast<Creature*>(target)->GetCreatureProperties()->Type))
                        return SPELL_FAILED_BAD_TARGETS;
                }

                // There are many quest items which requires the caster to have tapped the mob
                if (GetSpellInfo()->hasAttributes(ATTRIBUTESEXB_CANT_TARGET_TAPPED) && target->IsTagged())
                {
                    if (target->GetTaggerGUID() != p_caster->GetGUID())
                    {
                        // We aren't the tagger, but we must check if we are in group and if one of the members have tagged the unit
                        if (p_caster->InGroup())
                        {
                            Player* plrTagger = p_caster->GetMapMgrPlayer(target->GetTaggerGUID());
                            if (!plrTagger || !p_caster->GetGroup()->HasMember(plrTagger))
                                return SPELL_FAILED_CANT_CAST_ON_TAPPED;
                        }
                        else
                            return SPELL_FAILED_CANT_CAST_ON_TAPPED;
                    }
                }

                if (GetSpellInfo()->HasEffect(SPELL_EFFECT_PICKPOCKET))
                {
                    if (target->IsPlayer())
                        return SPELL_FAILED_BAD_TARGETS;
                    else if (static_cast<Creature*>(target)->GetType() != UNIT_TYPE_UNDEAD && static_cast<Creature*>(target)->GetType() != UNIT_TYPE_HUMANOID)
                        return SPELL_FAILED_TARGET_NO_POCKETS;
                }

                // We can't disarm already unarmed unit
                if (GetSpellInfo()->MechanicsType == MECHANIC_DISARMED)
                {
                    if (target->IsPlayer())
                    {
                        uint8_t form = target->GetShapeShift();
                        if (form == FORM_CAT || form == FORM_TREE || form == FORM_TRAVEL || form == FORM_AQUA || form == FORM_BEAR || form == FORM_DIREBEAR || form == FORM_GHOSTWOLF)
                            return SPELL_FAILED_TARGET_NO_WEAPONS;

                        Item* mainWep = static_cast<Player*>(target)->GetItemInterface()->GetInventoryItem(EQUIPMENT_SLOT_MAINHAND);
                        if (!mainWep || mainWep->GetItemProperties()->Class != ITEM_CLASS_WEAPON || target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISARMED))
                            return SPELL_FAILED_TARGET_NO_WEAPONS;
                    }
                    else if (!target->GetEquippedItem(MELEE) || target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISARMED))
                        return SPELL_FAILED_TARGET_NO_WEAPONS;
                }

                // GM Flagged Players should be immune to other players' casts, but not their own.
                if (target->IsPlayer() && (static_cast<Player*>(target)->HasFlag(PLAYER_FLAGS, PLAYER_FLAG_GM) || static_cast<Player*>(target)->m_isGmInvisible))
                    return SPELL_FAILED_BM_OR_INVISGOD;

                // Aura scaling
                // Correct spell was set in HandleCastSpellOpcode but we must recheck it so we can send proper error message if target is too low
                if (!i_caster && !m_triggeredSpell)
                {
                    if (GetSpellInfo() != GetSpellInfo()->getAuraRankForUnitLevel(target->getLevel()))
                        return SPELL_FAILED_LOWLEVEL;
                }
            }

            // Cannot cast channelled spells on totems
            // This solution is from MaNGOS
            // check could be done for all channeled spells but I think there might exist some quest spells or normal spells which you can cast to totems
            if (GetSpellInfo()->hasAttributes(ATTRIBUTESEX_CHANNEL_FACE_TARGET) && GetSpellInfo()->hasAttributes(ATTRIBUTESEXE_HASTE_AFFECTS_DURATION) && target->IsTotem())
                return SPELL_FAILED_IMMUNE;

            // Facing checks - "Must be behind the target"
            if (GetSpellInfo()->AttributesExB == ATTRIBUTESEXB_REQ_BEHIND_TARGET && GetSpellInfo()->hasAttributes(ATTRIBUTESEX_REQ_FACING_TARGET) && target->isInFront(m_caster))
            {
                // Some spells have their old attributes still in place
                // TODO: MOVE THESE TO SPELL HANDLER LIBRARY (expect throw)
                if (!(GetSpellInfo()->spellIconID == 495 && GetSpellInfo()->SpellFamilyName == 7) &&    // Druid - Pounce, "Patch 2.0.3 - Pounce no longer requires the druid to be behind the target."
                    !(GetSpellInfo()->spellIconID == 2117 && GetSpellInfo()->SpellFamilyName == 8) &&   // Rogue - Mutilate, "Patch 3.0.2 - Mutilate no longer requires you be behind the target."
                    GetSpellInfo()->Id != 2764)                                                         // Throw spell has wrong attributes set, wtf Blizz??
                    return SPELL_FAILED_NOT_BEHIND;
            }

            // Facing checks - "Must be in front of target"
            if ((GetSpellInfo()->Attributes == (ATTRIBUTES_ABILITY | ATTRIBUTES_NOT_SHAPESHIFT | ATTRIBUTES_UNK20 | ATTRIBUTES_STOP_ATTACK) && !target->isInFront(m_caster)) ||
                (GetSpellInfo()->hasAttributes(ATTRIBUTESEX_REQ_FACING_TARGET) && !m_caster->isInFront(target)))
                return SPELL_FAILED_NOT_INFRONT;

            if (!(m_targets.m_targetMask & (TARGET_FLAG_CORPSE | TARGET_FLAG_UNIT_CORPSE | TARGET_FLAG_CORPSE2) || GetSpellInfo()->hasAttributes(ATTRIBUTESEXB_CAN_CAST_ON_DEAD_UNIT)) &&
                !target->isAlive())
                return SPELL_FAILED_TARGETS_DEAD;

            if (target->hasAuraWithAuraType(SPELL_AURA_SPIRIT_OF_REDEMPTION))
                return SPELL_FAILED_BAD_TARGETS;

            // Line of Sight check
            if (worldConfig.terrainCollision.isCollisionEnabled)
            {
                if (m_caster->GetMapId() == target->GetMapId() &&
                    !m_caster->GetMapMgr()->isInLineOfSight(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), target->GetPositionX(), target->GetPositionY(), target->GetPositionZ()))
                    return SPELL_FAILED_LINE_OF_SIGHT;
            }

            if (GetSpellInfo()->hasAttributes(ATTRIBUTESEXC_ONLY_PLAYER_TARGETS) && !target->IsPlayer())
                return SPELL_FAILED_TARGET_NOT_PLAYER;

            if (target->hasAuraWithAuraType(SPELL_AURA_PREVENT_RESURRECTION))
            {
                if (GetSpellInfo()->HasEffect(SPELL_EFFECT_RESURRECT) || GetSpellInfo()->HasEffect(SPELL_EFFECT_SELF_RESURRECT) || GetSpellInfo()->HasEffect(SPELL_EFFECT_RESURRECT_FLAT))
                    return SPELL_FAILED_TARGET_CANNOT_BE_RESURRECTED;
            }
        }
    }

    // Check Line of Sight for spells with a destination
    if (m_targets.hasDestination() && worldConfig.terrainCollision.isCollisionEnabled)
    {
        if (!m_caster->GetMapMgr()->isInLineOfSight(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), m_targets.destination().x, m_targets.destination().y, m_targets.destination().z))
            return SPELL_FAILED_LINE_OF_SIGHT;
    }

    // Check do we have (alive) pet if spell target is a pet
    if (p_caster)
    {
        for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            if (GetSpellInfo()->EffectImplicitTargetA[i] == EFF_TARGET_PET)
            {
                Pet* pet = p_caster->GetSummon();
                if (!pet)
                {
                    if (m_triggeredByAura)
                        return SPELL_FAILED_DONT_REPORT;
                    else
                        return SPELL_FAILED_NO_PET;
                }
                else if (!pet->isAlive())
                    return SPELL_FAILED_TARGETS_DEAD;
            }
        }
    }

    // Area checks
    if (p_caster)
    {
        if (GetSpellInfo()->RequiresAreaId > 0)
        {
            bool found = false;
            auto areaGroup = sAreaGroupStore.LookupEntry(GetSpellInfo()->RequiresAreaId);
            auto areaEntry = p_caster->GetArea();
            while (areaGroup)
            {
                for (uint8 i = 0; i < 6; ++i)
                {
                    if (areaGroup->AreaId[i] == areaEntry->id || (areaEntry->zone != 0 && areaGroup->AreaId[i] == areaEntry->zone))
                        found = true;
                }
                if (found || !areaGroup->next_group)
                    break;
                areaGroup = sAreaGroupStore.LookupEntry(areaGroup->next_group);
            }

            if (!found)
                return SPELL_FAILED_INCORRECT_AREA;
        }

        // Flying mount check
        if (GetSpellInfo()->hasAttributes(ATTRIBUTESEXD_ONLY_IN_OUTLANDS))
        {
            auto areaEntry = p_caster->GetArea();
            if (!areaEntry)
                areaEntry = sAreaStore.LookupEntry(p_caster->GetZoneId());
            if (!areaEntry || !(areaEntry->flags & MapManagement::AreaManagement::AreaFlags::AREA_FLAG_OUTLAND && !(areaEntry->flags & MapManagement::AreaManagement::AreaFlags::AREA_FLAG_NO_FLY_ZONE)) ||
                (p_caster->GetMapId() == 571 && !(GetSpellInfo()->hasAttributes(ATTRIBUTESEXG_IGNORE_COLD_WEATHER_FLYING) || p_caster->HasSpell(54197)))) // Cold Weather Flying
                return SPELL_FAILED_INCORRECT_AREA;
        }

        // Raid and heroic instance exclusive spells (in 3.3.5a only spells with this attribute are "Sayge's Dark Fortune of X")
        if (GetSpellInfo()->hasAttributes(ATTRIBUTESEXF_NOT_IN_RAID_OR_HEROIC_DUNGEONS) && p_caster->GetMapMgr())
        {
            if (p_caster->GetMapMgr()->iInstanceMode == MODE_HEROIC || p_caster->GetMapMgr()->GetMapInfo()->type == INSTANCE_RAID)
                return SPELL_FAILED_NOT_IN_RAID_INSTANCE;
        }
    }

    // You cannot cast spells while on taxi or mount
    if (p_caster && p_caster->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_MOUNT) && !m_triggeredSpell && !GetSpellInfo()->IsPassive() && !GetSpellInfo()->hasAttributes(ATTRIBUTES_MOUNT_CASTABLE))
    {
        if (p_caster->GetTaxiState())
            return SPELL_FAILED_NOT_ON_TAXI;
        else
            return SPELL_FAILED_NOT_MOUNTED;
    }

    // Item checks
    // TODO: This part may not work correctly right now, needs further investigation/work
    if (!GetSpellInfo()->IsPassive() && (m_targets.m_itemTarget || i_caster) && p_caster)
    {
        if (GetSpellInfo()->hasAttributes(ATTRIBUTESEXB_IGNORE_ITEM_CHECKS))
            return SPELL_CANCAST_OK;

        bool scrollItem = false; // if spell is casted from a enchant scroll
        bool vellumTarget = false; // if spell is casted to either armor or weapon vellum

                                   // If the spell is casted by an item
        if (i_caster)
        {
            // TODO: check is the cast item in trade window. if true, return error msg
            if (!p_caster->HasItemCount(i_caster->GetEntry(), 1))
                return SPELL_FAILED_ITEM_NOT_FOUND;

            ItemProperties const* proto = i_caster->GetItemProperties();
            if (!proto)
                return SPELL_FAILED_ITEM_NOT_FOUND;

            if (proto->Flags & ITEM_FLAG_NO_REAGENT)
                scrollItem = true;

            // Check does item have charges left
            for (int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
            {
                if (proto->Spells[i].Charges && i_caster->GetCharges(i) == 0)
                    return SPELL_FAILED_NO_CHARGES_REMAIN;
            }

            // Potions, healthstones, mana items etc
            Unit* targetUnit = m_caster->GetMapMgrUnit(m_targets.m_unitTarget);
            if (proto->Class == ITEM_CLASS_CONSUMABLE && targetUnit)
            {
                SpellCastResult errorMessage = SPELL_CANCAST_OK;
                for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
                {
                    // Pet related effects are handled later
                    if (GetSpellInfo()->EffectImplicitTargetA[i] == EFF_TARGET_PET)
                        continue;

                    // +HP items
                    if (GetSpellInfo()->Effect[i] == SPELL_EFFECT_HEAL)
                    {
                        if (targetUnit->GetHealth() == targetUnit->GetMaxHealth())
                        {
                            errorMessage = SPELL_FAILED_ALREADY_AT_FULL_HEALTH;
                            continue;
                        }
                        else
                        {
                            errorMessage = SPELL_CANCAST_OK;
                            break;
                        }
                    }

                    // +Mana/Energy items
                    if (GetSpellInfo()->Effect[i] == SPELL_EFFECT_ENERGIZE)
                    {
                        if (GetSpellInfo()->EffectMiscValue[i] < 0 || GetSpellInfo()->EffectMiscValue[i] > POWER_TYPE_RUNIC_POWER)
                        {
                            errorMessage = SPELL_FAILED_ALREADY_AT_FULL_POWER;
                            continue;
                        }

                        uint8_t power = uint8_t(GetSpellInfo()->EffectMiscValue[i]);
                        if (targetUnit->GetPower(power) == targetUnit->GetMaxPower(power))
                        {
                            errorMessage = (power == POWER_TYPE_MANA) ? SPELL_FAILED_ALREADY_AT_FULL_MANA : SPELL_FAILED_ALREADY_AT_FULL_POWER;
                            continue;
                        }
                        else
                        {
                            errorMessage = SPELL_CANCAST_OK;
                            break;
                        }
                    }
                }
                if (errorMessage != SPELL_CANCAST_OK)
                    return errorMessage;
            }
        }

        // If spell is casted on item
        if (m_targets.m_itemTarget)
        {
            Item* targetItem = nullptr;
            if (m_targets.m_targetMask & TARGET_FLAG_TRADE_ITEM)
            {
                if (GetSpellInfo()->Effect[0] && (GetSpellInfo()->Effect[0] == SPELL_EFFECT_OPEN_LOCK ||
                    GetSpellInfo()->Effect[0] == SPELL_EFFECT_ENCHANT_ITEM ||
                    GetSpellInfo()->Effect[0] == SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY))
                {
                    if (GetSpellInfo()->hasAttributes(ATTRIBUTESEXB_ENCHANT_OWN_ONLY))
                        return SPELL_FAILED_BAD_TARGETS;

                    if (p_caster->GetTradeTarget())
                        targetItem = p_caster->GetTradeTarget()->getTradeItem(uint32(m_targets.m_itemTarget));
                }
            }
            else
                targetItem = p_caster->GetItemInterface()->GetItemByGUID(m_targets.m_itemTarget);

            if (!targetItem)
                return SPELL_FAILED_ITEM_GONE;

            if (!targetItem->fitsToSpellRequirements(GetSpellInfo()))
                return SPELL_FAILED_EQUIPPED_ITEM_CLASS;

            if ((GetSpellInfo()->EquippedItemClass == ITEM_CLASS_ARMOR && targetItem->GetItemProperties()->Class == ITEM_CLASS_TRADEGOODS && targetItem->GetItemProperties()->SubClass == ITEM_SUBCLASS_ARMOR_ENCHANTMENT) ||
                (GetSpellInfo()->EquippedItemClass == ITEM_CLASS_WEAPON && targetItem->GetItemProperties()->Class == ITEM_CLASS_TRADEGOODS && targetItem->GetItemProperties()->SubClass == ITEM_SUBCLASS_WEAPON_ENCHANTMENT))
                vellumTarget = true;
        }
        else if (!m_targets.m_itemTarget && GetSpellInfo()->EquippedItemClass > 0)
        {
            bool isProperType = false;
            switch (GetSpellInfo()->EquippedItemClass)
            {
            case ITEM_CLASS_WEAPON:
            {
                for (uint8_t i = EQUIPMENT_SLOT_MAINHAND; i < EQUIPMENT_SLOT_TABARD; ++i)
                {
                    if (Item* item = p_caster->GetItemInterface()->GetInventoryItem(i))
                    {
                        // Check for disarms
                        if ((i == EQUIPMENT_SLOT_MAINHAND && p_caster->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISARMED)) ||
                            (i == EQUIPMENT_SLOT_OFFHAND && p_caster->HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_DISARM_OFFHAND)) ||
                            (i == EQUIPMENT_SLOT_RANGED && p_caster->HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_DISARM_RANGED)))
                            continue;
                        if (item->fitsToSpellRequirements(GetSpellInfo()))
                        {
                            isProperType = true;
                            break;
                        }
                    }
                }
                break;
            }
            case ITEM_CLASS_ARMOR:
            {
                for (uint8_t i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_MAINHAND; ++i)
                {
                    if (Item* item = p_caster->GetItemInterface()->GetInventoryItem(i))
                    {
                        if (item->fitsToSpellRequirements(GetSpellInfo()))
                        {
                            isProperType = true;
                            break;
                        }
                    }
                }

                // No need to continue further if matched already
                if (isProperType)
                    break;

                // Shields are classified as armor
                if (Item* item = p_caster->GetItemInterface()->GetInventoryItem(EQUIPMENT_SLOT_OFFHAND))
                {
                    if (item->fitsToSpellRequirements(GetSpellInfo()) && !p_caster->HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_DISARM_OFFHAND))
                    {
                        isProperType = true;
                        break;
                    }
                }

                // Ranged slot can have items classified as armor (no need to check for disarm in these cases)
                if (Item* item = p_caster->GetItemInterface()->GetInventoryItem(EQUIPMENT_SLOT_RANGED))
                {
                    if (item->fitsToSpellRequirements(GetSpellInfo()))
                    {
                        isProperType = true;
                        break;
                    }
                }
                break;
            }
            default:
                break;
            }

            if (!isProperType)
                return SPELL_FAILED_EQUIPPED_ITEM_CLASS;
        }

        if (!(i_caster && i_caster->GetItemProperties()->Flags & ITEM_FLAG_NO_REAGENT))
        {
            // Reagent checks
            bool checkForReagents = !(GetSpellInfo()->hasAttributes(ATTRIBUTESEXE_REAGENT_REMOVAL) && p_caster->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NO_REAGANT_COST));
            // Check for mask also
            if (checkForReagents &&
                ((GetSpellInfo()->SpellGroupType[0] && (GetSpellInfo()->SpellGroupType[0] & p_caster->getUInt32Value(PLAYER_NO_REAGENT_COST_1))) ||
                (GetSpellInfo()->SpellGroupType[1] && (GetSpellInfo()->SpellGroupType[1] & p_caster->getUInt32Value(PLAYER_NO_REAGENT_COST_1 + 1))) ||
                    (GetSpellInfo()->SpellGroupType[2] && (GetSpellInfo()->SpellGroupType[2] & p_caster->getUInt32Value(PLAYER_NO_REAGENT_COST_1 + 2)))))
                checkForReagents = false;
            // If item is in trade window, reagents will be used
            if (!checkForReagents && m_targets.m_itemTarget && (m_targets.m_targetMask & TARGET_FLAG_TRADE_ITEM))
                checkForReagents = true;

            if (checkForReagents)
            {
                for (uint32_t i = 0; i < MAX_SPELL_REAGENTS; ++i)
                {
                    if (GetSpellInfo()->Reagent[i] <= 0)
                        continue;

                    uint32_t itemId = GetSpellInfo()->Reagent[i];
                    uint32_t itemCount = GetSpellInfo()->ReagentCount[i];
                    // If the item is used as spell reagent
                    if (i_caster && i_caster->GetEntry() == itemId)
                    {
                        ItemProperties const* proto = i_caster->GetItemProperties();
                        if (!proto)
                            return SPELL_FAILED_ITEM_NOT_FOUND;
                        for (uint8_t x = 0; x < MAX_ITEM_PROTO_SPELLS; ++x)
                        {
                            if (proto->Spells[x].Charges < 0 && i_caster->GetCharges(x) < 2)
                            {
                                ++itemCount;
                                break;
                            }
                        }
                    }
                    if (!p_caster->HasItemCount(itemId, itemCount))
                    {
                        if (parameter1)
                            *parameter1 = itemId;
                        return SPELL_FAILED_REAGENTS;
                    }
                }
            }

            // Check for totem items
            uint8_t totems = 2;
            for (uint8_t i = 0; i < MAX_SPELL_TOTEMS; ++i)
            {
                if (GetSpellInfo()->Totem[i] != 0)
                {
                    if (p_caster->HasItemCount(GetSpellInfo()->Totem[i], 1))
                    {
                        totems -= 1;
                        continue;
                    }
                }
                else
                    totems -= 1;
            }
            if (totems != 0)
                return SPELL_FAILED_TOTEMS;

            // Check for totem category items
            uint8_t totemCategory = 2;
            for (uint8_t i = 0; i < MAX_SPELL_TOTEM_CATEGORIES; ++i)
            {
                if (GetSpellInfo()->TotemCategory[i] != 0)
                {
                    if (p_caster->HasItemCount(GetSpellInfo()->TotemCategory[i], 1))
                    {
                        totemCategory -= 1;
                        continue;
                    }
                }
                else
                    totemCategory -= 1;
            }
            if (totemCategory != 0)
                return SPELL_FAILED_TOTEM_CATEGORY;

            // Special checks for spell effects
            for (uint8_t i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                switch (GetSpellInfo()->Effect[i])
                {
                case SPELL_EFFECT_CREATE_ITEM:
                case SPELL_EFFECT_CREATE_ITEM2:
                {
                    if (!m_triggeredSpell && GetSpellInfo()->EffectItemType[i])
                    {
                        ItemProperties const* proto = sMySQLStore.getItemProperties(GetSpellInfo()->EffectItemType[i]);
                        if (!proto)
                            return SPELL_FAILED_ITEM_NOT_READY; // should not happen anyway
                        if (p_caster->GetItemInterface()->CalculateFreeSlots(proto) == 0)
                        {
                            p_caster->GetItemInterface()->BuildInventoryChangeError(nullptr, nullptr, INV_ERR_INVENTORY_FULL);
                            return SPELL_FAILED_DONT_REPORT;
                        }
                        const int8_t msg = p_caster->GetItemInterface()->CanReceiveItem(proto, 1);
                        if (msg != INV_ERR_OK)
                        {
                            p_caster->GetItemInterface()->BuildInventoryChangeError(nullptr, nullptr, msg);
                            return SPELL_FAILED_DONT_REPORT;
                        }
                    }
                    break;
                }
                case SPELL_EFFECT_ENCHANT_ITEM:
                {
                    if (GetSpellInfo()->EffectItemType[i] && m_targets.m_itemTarget && vellumTarget)
                    {
                        // you can only enchant your own vellums
                        if (m_targets.m_targetMask & TARGET_FLAG_TRADE_ITEM)
                            return SPELL_FAILED_NOT_TRADEABLE;
                        // prevent duping the vellum into other vellums
                        if (scrollItem)
                            return SPELL_FAILED_TOTEM_CATEGORY;
                        ItemProperties const* proto = sMySQLStore.getItemProperties(GetSpellInfo()->EffectItemType[i]);
                        if (!proto)
                            return SPELL_FAILED_ITEM_NOT_READY; // should not happen anyway
                        if (p_caster->GetItemInterface()->CalculateFreeSlots(proto) == 0)
                        {
                            p_caster->GetItemInterface()->BuildInventoryChangeError(nullptr, nullptr, INV_ERR_INVENTORY_FULL);
                            return SPELL_FAILED_DONT_REPORT;
                        }
                        const int8_t msg = p_caster->GetItemInterface()->CanReceiveItem(proto, 1);
                        if (msg != INV_ERR_OK)
                        {
                            p_caster->GetItemInterface()->BuildInventoryChangeError(nullptr, nullptr, msg);
                            return SPELL_FAILED_DONT_REPORT;
                        }
                    }
                }
                // no need for break here
                case SPELL_EFFECT_ADD_SOCKET:
                {
                    Item* targetItem = nullptr;
                    if (m_targets.m_targetMask & TARGET_FLAG_TRADE_ITEM)
                    {
                        if (p_caster->GetTradeTarget())
                            targetItem = p_caster->GetTradeTarget()->getTradeItem(uint32(m_targets.m_itemTarget));
                    }
                    else
                        targetItem = p_caster->GetItemInterface()->GetItemByGUID(m_targets.m_itemTarget);

                    if (!targetItem)
                        return SPELL_FAILED_ITEM_NOT_FOUND;

                    if (targetItem->GetItemProperties()->ItemLevel < GetSpellInfo()->baseLevel)
                        return SPELL_FAILED_LOWLEVEL;

                    bool isUseItem = false;
                    for (uint8_t x = 0; x < MAX_ITEM_PROTO_SPELLS; ++x)
                    {
                        ItemProperties const* proto = targetItem->GetItemProperties();
                        if (proto->Spells[x].Id > 0 &&
                            (proto->Spells[x].Trigger == USE ||
                                proto->Spells[x].Trigger == APPLY_AURA_ON_PICKUP))
                        {
                            isUseItem = true;
                            break;
                        }
                    }

                    // You are not able to add enchantments to item which has on use effect already
                    auto enchantEntry = sSpellItemEnchantmentStore.LookupEntry(GetSpellInfo()->EffectMiscValue[i]);
                    if (enchantEntry)
                    {
                        for (uint8_t x = 0; x < 3; ++x)
                        {
                            switch (enchantEntry->type[x])
                            {
                            case 7: // Enchants 'on use' enchantment to item
                                if (isUseItem)
                                    return SPELL_FAILED_ON_USE_ENCHANT;
                                break;
                            case 8: // Enchants new prismatic socket slot to item
                            {
                                uint32_t sockets = 0;
                                for (uint32_t socket = 0; socket < MAX_ITEM_PROTO_SOCKETS; ++socket)
                                {
                                    if (targetItem->GetItemProperties()->Sockets[socket].SocketColor)
                                        ++sockets;
                                }

                                if (sockets == MAX_ITEM_PROTO_SOCKETS || targetItem->GetEnchantmentId(PRISMATIC_ENCHANTMENT_SLOT))
                                    return SPELL_FAILED_MAX_SOCKETS;
                                break;
                            }
                            default:
                                break;
                            }
                        }
                    }

                    // Check item owner in some cases
                    if (targetItem->GetOwner() != p_caster)
                    {
                        if (!enchantEntry)
                            return SPELL_FAILED_ERROR;
                        if (enchantEntry->EnchantGroups & 0x01) // makes item soulbound
                            return SPELL_FAILED_NOT_TRADEABLE;
                    }
                    break;
                }
                case SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY:
                {
                    Item* targetItem = nullptr;
                    if (m_targets.m_targetMask & TARGET_FLAG_TRADE_ITEM)
                    {
                        if (p_caster->GetTradeTarget())
                            targetItem = p_caster->GetTradeTarget()->getTradeItem(uint32_t(m_targets.m_itemTarget));
                    }
                    else
                        targetItem = p_caster->GetItemInterface()->GetItemByGUID(m_targets.m_itemTarget);

                    if (!targetItem)
                        return SPELL_FAILED_ITEM_NOT_FOUND;

                    // Check item owner in some cases
                    if (targetItem->GetOwner() != p_caster)
                    {
                        uint32_t enchantId = GetSpellInfo()->EffectMiscValue[i];
                        auto itemEnchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);
                        if (!itemEnchant)
                            return SPELL_FAILED_ERROR;
                        if (itemEnchant->EnchantGroups & 0x01) // makes item soulbound
                            return SPELL_FAILED_NOT_TRADEABLE;
                    }
                    break;
                }
                case SPELL_EFFECT_DISENCHANT:
                {
                    Item* targetItem = nullptr;
                    if (m_targets.m_targetMask & TARGET_FLAG_TRADE_ITEM)
                    {
                        if (p_caster->GetTradeTarget())
                            targetItem = p_caster->GetTradeTarget()->getTradeItem(uint32_t(m_targets.m_itemTarget));
                    }
                    else
                        targetItem = p_caster->GetItemInterface()->GetItemByGUID(m_targets.m_itemTarget);

                    if (!targetItem)
                        return SPELL_FAILED_CANT_BE_DISENCHANTED;

                    // Prevent disenchanting in trade
                    if (targetItem->GetOwnerGUID() != p_caster->GetGUID())
                        return SPELL_FAILED_CANT_BE_DISENCHANTED;

                    ItemProperties const* proto = targetItem->GetItemProperties();
                    if (!proto)
                        return SPELL_FAILED_CANT_BE_DISENCHANTED;

                    if (proto->Class != ITEM_CLASS_WEAPON && proto->Class != ITEM_CLASS_ARMOR)
                        return SPELL_FAILED_CANT_BE_DISENCHANTED;
                    if (proto->Quality > 4 || proto->Quality < 2)
                        return SPELL_FAILED_CANT_BE_DISENCHANTED;
                    // As of patch 2.0.1 disenchanting item requires certain skill level
                    // TODO: move to Spell Handler so it does not affect pretbc ascemu
                    const int32_t disenchantingSkill = proto->DisenchantReqSkill;
                    if (disenchantingSkill == -1)
                        return SPELL_FAILED_CANT_BE_DISENCHANTED;
                    if (disenchantingSkill > p_caster->_GetSkillLineCurrent(SKILL_ENCHANTING))
                        return SPELL_FAILED_LOW_CASTLEVEL;
                    // TODO: check does item have disenchant loot
                    break;
                }
                case SPELL_EFFECT_PROSPECTING:
                {
                    Item* targetItem = nullptr;
                    if (m_targets.m_targetMask & TARGET_FLAG_TRADE_ITEM)
                    {
                        if (p_caster->GetTradeTarget())
                            targetItem = p_caster->GetTradeTarget()->getTradeItem(uint32_t(m_targets.m_itemTarget));
                    }
                    else
                        targetItem = p_caster->GetItemInterface()->GetItemByGUID(m_targets.m_itemTarget);

                    if (!targetItem)
                        return SPELL_FAILED_CANT_BE_PROSPECTED;

                    if (!(targetItem->GetItemProperties()->Flags & ITEM_FLAG_PROSPECTABLE))
                        return SPELL_FAILED_CANT_BE_PROSPECTED;

                    // Prevent prospecting in trade
                    if (targetItem->GetOwnerGUID() != p_caster->GetGUID())
                        return SPELL_FAILED_CANT_BE_PROSPECTED;

                    // Does player have enough skill to prospect the ores?
                    if (targetItem->GetItemProperties()->RequiredSkillRank > p_caster->_GetSkillLineCurrent(SKILL_JEWELCRAFTING))
                        return SPELL_FAILED_LOW_CASTLEVEL;

                    // Does player have enough ores for prospect?
                    if (targetItem->GetStackCount() < 5)
                    {
                        if (parameter1 && parameter2)
                        {
                            *parameter1 = targetItem->GetEntry();
                            *parameter2 = 5;
                        }
                        return SPELL_FAILED_NEED_MORE_ITEMS;
                    }

                    // TODO: check does item have prospecting loot
                    break;
                }
                case SPELL_EFFECT_MILLING:
                {
                    Item* targetItem = nullptr;
                    if (m_targets.m_targetMask & TARGET_FLAG_TRADE_ITEM)
                    {
                        if (p_caster->GetTradeTarget())
                            targetItem = p_caster->GetTradeTarget()->getTradeItem(uint32_t(m_targets.m_itemTarget));
                    }
                    else
                        targetItem = p_caster->GetItemInterface()->GetItemByGUID(m_targets.m_itemTarget);

                    if (!targetItem)
                        return SPELL_FAILED_CANT_BE_MILLED;

                    if (!(targetItem->GetItemProperties()->Flags & ITEM_FLAG_MILLABLE))
                        return SPELL_FAILED_CANT_BE_MILLED;

                    // Prevent milling in trade
                    if (targetItem->GetOwnerGUID() != p_caster->GetGUID())
                        return SPELL_FAILED_CANT_BE_MILLED;

                    // Does player have enough skill to mill the herbs?
                    if (targetItem->GetItemProperties()->RequiredSkillRank > p_caster->_GetSkillLineCurrent(SKILL_INSCRIPTION))
                        return SPELL_FAILED_LOW_CASTLEVEL;

                    // Does player have enough herbs for milling?
                    if (targetItem->GetStackCount() < 5)
                    {
                        if (parameter1 && parameter2)
                        {
                            *parameter1 = targetItem->GetEntry();
                            *parameter2 = 5;
                        }
                        return SPELL_FAILED_NEED_MORE_ITEMS;
                    }

                    // TODO: check does the item have milling loot
                    break;
                }
                case SPELL_EFFECT_WEAPON_DAMAGE:
                case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
                {
                    // Check if the item is ranged weapon or wand
                    if (getSpellDamageType() != RANGED)
                        break;

                    Item* rangedWeapon = p_caster->GetItemInterface()->GetInventoryItem(EQUIPMENT_SLOT_RANGED);
                    if (!rangedWeapon || rangedWeapon->GetItemProperties()->Class != ITEM_CLASS_WEAPON || (rangedWeapon->GetDurabilityMax() > 0 && rangedWeapon->GetDurability() == 0))
                        return SPELL_FAILED_EQUIPPED_ITEM;

                    switch (rangedWeapon->GetItemProperties()->SubClass)
                    {
                    case ITEM_SUBCLASS_WEAPON_THROWN:
                        if (!p_caster->HasItemCount(rangedWeapon->GetEntry(), 1))
                            return SPELL_FAILED_NO_AMMO;
                        break;
                    case ITEM_SUBCLASS_WEAPON_BOW:
                    case ITEM_SUBCLASS_WEAPON_GUN:
                    case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                    {
                        const uint32_t ammo = p_caster->GetAmmoId();
                        if (!ammo)
                        {
                            // Thori'dal, the Stars' Fury (dummy aura) - Generates magical arrows, does not require ammo
                            // only item with this kind of effect?
                            if (p_caster->HasAura(46699))
                                break;
                            return SPELL_FAILED_NO_AMMO;
                        }

                        ItemProperties const* ammoProto = sMySQLStore.getItemProperties(ammo);
                        if (!ammoProto)
                            return SPELL_FAILED_NO_AMMO;

                        if (ammoProto->Class != ITEM_CLASS_PROJECTILE)
                            return SPELL_FAILED_NO_AMMO;

                        // Check for correct projectile for weapon
                        if (rangedWeapon->GetItemProperties()->SubClass == ITEM_SUBCLASS_WEAPON_BOW || rangedWeapon->GetItemProperties()->SubClass == ITEM_SUBCLASS_WEAPON_CROSSBOW)
                        {
                            if (ammoProto->SubClass != ITEM_SUBCLASS_PROJECTILE_ARROW)
                                return SPELL_FAILED_NO_AMMO;
                        }
                        else
                        {
                            if (ammoProto->SubClass != ITEM_SUBCLASS_PROJECTILE_BULLET)
                                return SPELL_FAILED_NO_AMMO;
                        }

                        if (!p_caster->HasItemCount(ammo, 1))
                        {
                            p_caster->SetAmmoId(0);
                            return SPELL_FAILED_NO_AMMO;
                        }
                        break;
                    }
                    default:
                        break;
                    }
                    break;
                }
                case SPELL_EFFECT_CREATE_MANA_GEM:
                {
                    const uint32_t itemId = GetSpellInfo()->EffectItemType[i];
                    ItemProperties const* itemProto = sMySQLStore.getItemProperties(itemId);

                    if (!itemProto)
                        return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

                    if (Item* itemGem = p_caster->GetItemInterface()->getItemById(itemId))
                    {
                        for (int x = 0; x < MAX_ITEM_PROTO_SPELLS; ++x)
                        {
                            if (itemProto->Spells[x].Charges != 0 && itemGem->GetCharges(x) == itemProto->Spells[x].Charges)
                                return SPELL_FAILED_ITEM_AT_MAX_CHARGES;
                        }
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }
    }

    if (!m_triggeredSpell)
    {
        // Caster state check
        SpellCastResult const stateCheck = checkCasterState();
        if (stateCheck != SPELL_CANCAST_OK)
            return stateCheck;

        // Spell range check
        SpellCastResult const rangeCheck = checkRange(tolerate);
        if (rangeCheck != SPELL_CANCAST_OK)
            return rangeCheck;
    }

    // Spell cost check
    SpellCastResult const costCheck = HasPower();
    if (costCheck != SPELL_CANCAST_OK)
        return costCheck;

    // TODO: next, loop through spell effecs? and implement spell focus

    // NOTE TO SELF: needs to be the last check, so it does not interfere with the checks in the beginning of the function
    if (p_caster)
    {
        // Check combo points
        if (!m_triggeredSpell && m_requiresCP && (!m_targets.m_unitTarget || m_targets.m_unitTarget != p_caster->m_comboTarget))
            return SPELL_FAILED_NO_COMBO_POINTS;
    }



    /// OLD CANCAST BELOW, REMOVE WHEN EACH CHECK IMPLEMENTED
    /*
    // Check if spell can be casted while player is moving.
    if ((p_caster != NULL) && p_caster->m_isMoving && (GetSpellInfo()->InterruptFlags & CAST_INTERRUPT_ON_MOVEMENT) && (m_castTime != 0) && (GetDuration() != 0))
        return SPELL_FAILED_MOVING;

    //Player caster checks
    
    if (p_caster)
    {

        //Indoor/Outdoor check
        
        if (worldConfig.terrainCollision.isCollisionEnabled)
        {
            if (GetSpellInfo()->MechanicsType == MECHANIC_MOUNTED)
            {
                if (!MapManagement::AreaManagement::AreaStorage::IsOutdoor(p_caster->GetMapId(), p_caster->GetPositionNC().x, p_caster->GetPositionNC().y, p_caster->GetPositionNC().z))
                    return SPELL_FAILED_NO_MOUNTS_ALLOWED;
            }
        }

        //Duel request check
        
        if (p_caster->GetDuelState() == DUEL_STATE_REQUESTED)
        {
            for (i = 0; i < 3; ++i)
            {
                if (GetSpellInfo()->Effect[i] && GetSpellInfo()->Effect[i] != SPELL_EFFECT_APPLY_AURA && GetSpellInfo()->Effect[i] != SPELL_EFFECT_APPLY_PET_AREA_AURA
                    && GetSpellInfo()->Effect[i] != SPELL_EFFECT_APPLY_GROUP_AREA_AURA && GetSpellInfo()->Effect[i] != SPELL_EFFECT_APPLY_RAID_AREA_AURA)
                {
                    return SPELL_FAILED_TARGET_DUELING;
                }
            }
        }

        //Duel area check
        if (GetSpellInfo()->Id == 7266)
        {
            auto at = p_caster->GetArea();
            if (at->flags & AREA_CITY_AREA)
                return SPELL_FAILED_NO_DUELING;
            // instance & stealth checks
            if (p_caster->GetMapMgr() && p_caster->GetMapMgr()->GetMapInfo() && p_caster->GetMapMgr()->GetMapInfo()->type != INSTANCE_NULL)
                return SPELL_FAILED_NO_DUELING;
            if (p_caster->isStealthed())
                return SPELL_FAILED_CANT_DUEL_WHILE_STEALTHED;
        }

        //On taxi check
        if (p_caster->m_onTaxi)
        {
            if (!hasAttributes(ATTRIBUTES_MOUNT_CASTABLE))    //Are mount castable spells allowed on a taxi?
            {
                if (GetSpellInfo()->Id != 33836 && GetSpellInfo()->Id != 45072 && GetSpellInfo()->Id != 45115 && GetSpellInfo()->Id != 31958)   // exception for taxi bombs
                    return SPELL_FAILED_NOT_ON_TAXI;
            }
        }
        else
        {
            if (GetSpellInfo()->Id == 33836 || GetSpellInfo()->Id == 45072 || GetSpellInfo()->Id == 45115 || GetSpellInfo()->Id == 31958)
                return SPELL_FAILED_NOT_HERE;
        }

        //Is mounted check
        if (!p_caster->IsMounted())
        {
            if (GetSpellInfo()->Id == 25860)  // Reindeer Transformation
                return SPELL_FAILED_ONLY_MOUNTED;
        }
        else
        {
            if (!hasAttributes(ATTRIBUTES_MOUNT_CASTABLE))
                return SPELL_FAILED_NOT_MOUNTED;
        }

        // check if spell is allowed while shapeshifted
        if (p_caster->GetShapeShift())
        {
            switch (p_caster->GetShapeShift())
            {
            case FORM_TREE:
            case FORM_BATTLESTANCE:
            case FORM_DEFENSIVESTANCE:
            case FORM_BERSERKERSTANCE:
            case FORM_SHADOW:
            case FORM_STEALTH:
            case FORM_MOONKIN:
            {
                break;
            }

            case FORM_SWIFT:
            case FORM_FLIGHT:
            {
                // check if item is allowed (only special items allowed in flight forms)
                if (i_caster && !(i_caster->GetItemProperties()->Flags & ITEM_FLAG_SHAPESHIFT_OK))
                    return SPELL_FAILED_NO_ITEMS_WHILE_SHAPESHIFTED;

                break;
            }

            //case FORM_CAT:
            //case FORM_TRAVEL:
            //case FORM_AQUA:
            //case FORM_BEAR:
            //case FORM_AMBIENT:
            //case FORM_GHOUL:
            //case FORM_DIREBEAR:
            //case FORM_CREATUREBEAR:
            //case FORM_GHOSTWOLF:

            case FORM_SPIRITOFREDEMPTION:
            {
                //Spirit of Redemption (20711) fix
                if (!(GetSpellInfo()->custom_c_is_flags & SPELL_FLAG_IS_HEALING) && GetSpellInfo()->Id != 7355)
                    return SPELL_FAILED_CASTER_DEAD;
                break;
            }


            default:
            {
                // check if item is allowed (only special & equipped items allowed in other forms)
                if (i_caster && !(i_caster->GetItemProperties()->Flags & ITEM_FLAG_SHAPESHIFT_OK))
                    if (i_caster->GetItemProperties()->InventoryType == INVTYPE_NON_EQUIP)
                        return SPELL_FAILED_NO_ITEMS_WHILE_SHAPESHIFTED;
            }
            }
        }


        //check if spell is allowed while we have a battleground flag
        if (p_caster->m_bgHasFlag)
        {
            switch (GetSpellInfo()->Id)
            {
                // stealth spells
            case 1784:
            case 1785:
            case 1786:
            case 1787:
            case 5215:
            case 6783:
            case 9913:
            case 1856:
            case 1857:
            case 26889:
            {
                // thank Cruders for this :P
                if (p_caster->m_bg && p_caster->m_bg->GetType() == BATTLEGROUND_WARSONG_GULCH)
                    p_caster->m_bg->HookOnFlagDrop(p_caster);
                else if (p_caster->m_bg && p_caster->m_bg->GetType() == BATTLEGROUND_EYE_OF_THE_STORM)
                    p_caster->m_bg->HookOnFlagDrop(p_caster);
                break;
            }
            }


        }


        //check if we have the required gameobject focus
        float focusRange;

        if (GetSpellInfo()->RequiresSpellFocus)
        {
            bool found = false;

            for (std::set<Object*>::iterator itr = p_caster->GetInRangeSetBegin(); itr != p_caster->GetInRangeSetEnd(); ++itr)
            {
                if (!(*itr)->IsGameObject())
                    continue;

                if ((static_cast<GameObject*>(*itr))->GetType() != GAMEOBJECT_TYPE_SPELL_FOCUS)
                    continue;

                if (!(p_caster->GetPhase() & (*itr)->GetPhase()))    //We can't see this, can't be the focus, skip further checks
                    continue;

                auto gameobject_info = static_cast<GameObject*>(*itr)->GetGameObjectProperties();
                if (!gameobject_info)
                {
                    LogDebugFlag(LF_SPELL, "Warning: could not find info about game object %u", (*itr)->GetEntry());
                    continue;
                }

                // professions use rangeIndex 1, which is 0yds, so we will use 5yds, which is standard interaction range.
                if (gameobject_info->raw.parameter_1)
                    focusRange = float(gameobject_info->raw.parameter_1);
                else
                    focusRange = GetMaxRange(sSpellRangeStore.LookupEntry(GetSpellInfo()->rangeIndex));

                // check if focus object is close enough
                if (!IsInrange(p_caster->GetPositionX(), p_caster->GetPositionY(), p_caster->GetPositionZ(), (*itr), (focusRange * focusRange)))
                    continue;

                if (gameobject_info->raw.parameter_0 == GetSpellInfo()->RequiresSpellFocus)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
                return SPELL_FAILED_REQUIRES_SPELL_FOCUS;
        }

    }

    
    //	set up our max range
    //	latency compensation!!
    //	figure out how much extra distance we need to allow for based on our
    //	movespeed and latency.
    float maxRange = 0;

    auto spell_range = sSpellRangeStore.LookupEntry(GetSpellInfo()->rangeIndex);
    if (spell_range != nullptr)
    {
        if (m_caster->IsInWorld())
        {
            Unit* target = m_caster->GetMapMgr()->GetUnit(m_targets.m_unitTarget);
            if (target != NULL && isFriendly(m_caster, target))
                maxRange = spell_range->maxRangeFriendly;
            else
                maxRange = spell_range->maxRange;
        }
        else
            maxRange = spell_range->maxRange;
    }

    if (u_caster && m_caster->GetMapMgr() && m_targets.m_unitTarget)
    {
        Unit* utarget = m_caster->GetMapMgr()->GetUnit(m_targets.m_unitTarget);

        if (utarget && utarget->IsPlayer() && static_cast<Player*>(utarget)->m_isMoving)
        {
            // this only applies to PvP.
            uint32 lat = static_cast<Player*>(utarget)->GetSession() ? static_cast<Player*>(utarget)->GetSession()->GetLatency() : 0;

            // if we're over 500 get fucked anyway.. your gonna lag! and this stops cheaters too
            lat = (lat > 500) ? 500 : lat;

            // calculate the added distance
            maxRange += u_caster->m_runSpeed * 0.001f * lat;
        }
    }

    //Some Unit caster range check
    if (u_caster != nullptr)
    {
        spellModFlatIntValue(u_caster->SM_FRange, &maxRange, GetSpellInfo()->SpellGroupType);
        spellModPercentageIntValue(u_caster->SM_PRange, &maxRange, GetSpellInfo()->SpellGroupType);
#ifdef COLLECTION_OF_UNTESTED_STUFF_AND_TESTERS
        float spell_flat_modifers = 0;
        float spell_pct_modifers = 0;
        spellModFlatIntValue(u_caster->SM_FRange, &spell_flat_modifers, GetProto()->SpellGroupType);
        spellModFlatIntValue(u_caster->SM_PRange, &spell_pct_modifers, GetProto()->SpellGroupType);
        if (spell_flat_modifers != 0 || spell_pct_modifers != 0)
            LOG_DEBUG("!!!!!spell range bonus mod flat %f , spell range bonus pct %f , spell range %f, spell group %u", spell_flat_modifers, spell_pct_modifers, maxRange, GetProto()->SpellGroupType);
#endif
    }

    // Targeted Location Checks (AoE spells)
    if (m_targets.m_targetMask == TARGET_FLAG_DEST_LOCATION)
    {
        if (!IsInrange(m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, m_caster, (maxRange * maxRange)))
            return SPELL_FAILED_OUT_OF_RANGE;
    }

    //Targeted Unit Checks
    if (m_targets.m_unitTarget)
    {
        Unit* target = (m_caster->IsInWorld()) ? m_caster->GetMapMgr()->GetUnit(m_targets.m_unitTarget) : NULL;

        if (target)
        {
            // UNIT_FIELD_BOUNDINGRADIUS + 1.5f; seems to match the client range

            if (tolerate)   // add an extra 33% to range on final check (squared = 1.78x)
            {
                float localrange = maxRange + target->GetBoundingRadius() + 1.5f;
                if (!IsInrange(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), target, (localrange * localrange * 1.78f)))
                    return SPELL_FAILED_OUT_OF_RANGE;
            }
            else
            {
                float localrange = maxRange + target->GetBoundingRadius() + 1.5f;
                if (!IsInrange(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), target, (localrange * localrange)))
                    return SPELL_FAILED_OUT_OF_RANGE;
            }

            if (p_caster != NULL)
            {

                if (GetSpellInfo()->Id == SPELL_RANGED_THROW)
                {
                    auto item = p_caster->GetItemInterface()->GetInventoryItem(EQUIPMENT_SLOT_RANGED);
                    if (item == nullptr)
                        return SPELL_FAILED_NO_AMMO;
                }

                if (target->IsPlayer())
                {
                    // disallow spell casting in sanctuary zones
                    // allow attacks in duels
                    if (p_caster->DuelingWith != target && !isFriendly(p_caster, target))
                    {
                        auto atCaster = p_caster->GetArea();
                        auto atTarget = target->GetArea();
                        if (atCaster->flags & 0x800 || atTarget->flags & 0x800)
                            return SPELL_FAILED_NOT_HERE;
                    }
                }
                else
                {
                    if (target->GetAIInterface()->GetIsSoulLinked() && u_caster && target->GetAIInterface()->getSoullinkedWith() != u_caster)
                        return SPELL_FAILED_BAD_TARGETS;
                }

                // pet training
                if (GetSpellInfo()->EffectImplicitTargetA[0] == EFF_TARGET_PET &&
                    GetSpellInfo()->Effect[0] == SPELL_EFFECT_LEARN_SPELL)
                {
                    Pet* pPet = p_caster->GetSummon();
                    // check if we have a pet
                    if (pPet == NULL)
                        return SPELL_FAILED_NO_PET;

                    // other checks
                    SpellInfo const* trig = sSpellCustomizations.GetSpellInfo(GetSpellInfo()->EffectTriggerSpell[0]);
                    if (trig == NULL)
                        return SPELL_FAILED_SPELL_UNAVAILABLE;

                    uint32 status = pPet->CanLearnSpell(trig);
                    if (status != 0)
                        return static_cast<uint8>(status);
                }

                if (GetSpellInfo()->EffectApplyAuraName[0] == SPELL_AURA_MOD_POSSESS)  //mind control
                {
                    if (GetSpellInfo()->EffectBasePoints[0])  //got level req;
                    {
                        if ((int32)target->getLevel() > GetSpellInfo()->EffectBasePoints[0] + 1 + int32(p_caster->getLevel() - GetSpellInfo()->spellLevel))
                            return SPELL_FAILED_HIGHLEVEL;
                        else if (target->IsCreature())
                        {
                            Creature* c = static_cast<Creature*>(target);
                            if (c->GetCreatureProperties()->Rank > ELITE_ELITE)
                                return SPELL_FAILED_HIGHLEVEL;
                        }
                    }
                }
            }

            // \todo Replace this awful hack with a better solution
            // Nestlewood Owlkin - Quest 9303
            if (GetSpellInfo()->Id == 29528 && target->IsCreature() && target->GetEntry() == 16518)
            {
                if (target->isRooted())
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }
                else
                {
                    target->SetTargetGUID(p_caster->GetGUID());
                    return SPELL_FAILED_SUCCESS;
                }

            }
            ////////////////////////////////////////////////////// Target check spells that are only castable on certain creatures/gameobjects ///////////////

            if (m_target_constraint != NULL)
            {
                // target is the wrong creature
                if (target->IsCreature() && !m_target_constraint->HasCreature(target->GetEntry()) && !m_target_constraint->IsFocused(target->GetEntry()))
                    return SPELL_FAILED_BAD_TARGETS;

                // target is the wrong GO :/
                if (target->IsGameObject() && !m_target_constraint->HasGameobject(target->GetEntry()) && !m_target_constraint->IsFocused(target->GetEntry()))
                    return SPELL_FAILED_BAD_TARGETS;

                bool foundTarget = false;
                Creature* pCreature = nullptr;
                size_t creatures = m_target_constraint->GetCreatures().size();

                // Spells for Invisibl Creatures and or Gameobjects ( Casting Spells Near them )
                for (size_t i = 0; i < creatures; ++i)
                {
                    if (!m_target_constraint->IsFocused(m_target_constraint->GetCreatures()[i]))
                    {
                        pCreature = m_caster->GetMapMgr()->GetInterface()->GetCreatureNearestCoords(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), m_target_constraint->GetCreatures()[i]);

                        if (pCreature)
                        {
                            if (pCreature->GetDistanceSq(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ()) <= 15)
                            {
                                SetTargetConstraintCreature(pCreature);
                                foundTarget = true;
                            }
                        }
                    }
                }

                GameObject* pGameobject = nullptr;
                size_t gameobjects = m_target_constraint->GetGameobjects().size();

                for (size_t i = 0; i < gameobjects; ++i)
                {
                    if (!m_target_constraint->IsFocused(m_target_constraint->GetGameobjects()[i]))
                    {
                        pGameobject = m_caster->GetMapMgr()->GetInterface()->GetGameObjectNearestCoords(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), m_target_constraint->GetGameobjects()[i]);

                        if (pGameobject)
                        {
                            if (pGameobject->GetDistanceSq(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ()) <= 15)
                            {
                                SetTargetConstraintGameObject(pGameobject);
                                foundTarget = true;
                            }
                        }
                    }
                }

                if (!foundTarget)
                    return SPELL_FAILED_BAD_TARGETS;
            }

            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            // scripted spell stuff
            switch (GetSpellInfo()->Id)
            {
            case 1515: // tame beast
            {
                uint8 result = 0;
                Unit* tgt = unitTarget;
                if (tgt == NULL)
                {
                    // we have to pick a target manually as this is a dummy spell which triggers tame effect at end of channeling
                    if (p_caster->GetSelection() != 0)
                        tgt = p_caster->GetMapMgr()->GetUnit(p_caster->GetSelection());
                    else
                        return SPELL_FAILED_UNKNOWN;
                }

                Creature* tame = tgt->IsCreature() ? static_cast<Creature*>(tgt) : NULL;

                if (tame == NULL)
                    result = PETTAME_INVALIDCREATURE;
                else if (!tame->isAlive())
                    result = PETTAME_DEAD;
                else if (tame->IsPet())
                    result = PETTAME_CREATUREALREADYOWNED;
                else if (tame->GetCreatureProperties()->Type != UNIT_TYPE_BEAST || !tame->GetCreatureProperties()->Family || !(tame->GetCreatureProperties()->Flags1 & CREATURE_FLAG1_TAMEABLE))
                    result = PETTAME_NOTTAMEABLE;
                else if (!p_caster->isAlive() || p_caster->getClass() != HUNTER)
                    result = PETTAME_UNITSCANTTAME;
                else if (tame->getLevel() > p_caster->getLevel())
                    result = PETTAME_TOOHIGHLEVEL;
                else if (p_caster->GetSummon() || p_caster->GetUnstabledPetNumber())
                    result = PETTAME_ANOTHERSUMMONACTIVE;
                else if (p_caster->GetPetCount() >= 5)
                    result = PETTAME_TOOMANY;
                else if (!p_caster->HasSpell(53270) && tame->IsExotic())
                    result = PETTAME_CANTCONTROLEXOTIC;
                else
                {
                    auto cf = sCreatureFamilyStore.LookupEntry(tame->GetCreatureProperties()->Family);
                    if (cf && !cf->tameable)
                        result = PETTAME_NOTTAMEABLE;
                }
                if (result != 0)
                {
                    SendTameFailure(result);
                    return SPELL_FAILED_DONT_REPORT;
                }
            }
            break;

            case 603: //curse of doom, can't be cast on players
            case 30910:
            case 47867: // Curse of doom rank 4
            {
                if (target->IsPlayer())
                    return SPELL_FAILED_TARGET_IS_PLAYER;
            }
            break;
            case 13907: // Smite Demon
            {
                if (target->IsPlayer() || target->getClass() != TARGET_TYPE_DEMON)
                    return SPELL_FAILED_SPELL_UNAVAILABLE;
            }
            break;

            default:
                break;
            }

            // if the target is not the unit caster and not the masters pet
            if (target != u_caster && !m_caster->IsPet())
            {

                // Inface checks, these are checked in 2 ways
                // 1e way is check for damage type, as 3 is always ranged
                // 2e way is trough the data in the extraspell db

                uint32 facing_flags = GetSpellInfo()->FacingCasterFlags;

                // Holy shock need enemies be in front of caster
                if (GetSpellInfo()->custom_NameHash == SPELL_HASH_HOLY_SHOCK && GetSpellInfo()->Effect[0] == SPELL_EFFECT_DUMMY && !isFriendly(u_caster, target))
                    facing_flags = SPELL_INFRONT_STATUS_REQUIRE_INFRONT;

                // burlex: units are always facing the target!
                if (p_caster && facing_flags != SPELL_INFRONT_STATUS_REQUIRE_SKIPCHECK)
                {
                    if (GetSpellInfo()->Spell_Dmg_Type == SPELL_DMG_TYPE_RANGED)
                    {
                        // our spell is a ranged spell
                        if (!p_caster->isInFront(target))
                            return SPELL_FAILED_UNIT_NOT_INFRONT;
                    }
                    else
                    {
                        // our spell is not a ranged spell
                        if (facing_flags == SPELL_INFRONT_STATUS_REQUIRE_INFRONT)
                        {
                            // must be in front
                            if (!u_caster->isInFront(target))
                                return SPELL_FAILED_UNIT_NOT_INFRONT;
                        }
                        else if (facing_flags == SPELL_INFRONT_STATUS_REQUIRE_INBACK)
                        {
                            // behind
                            if (target->isInFront(u_caster))
                                return SPELL_FAILED_NOT_BEHIND;
                        }
                    }
                }
            }

            if (GetSpellInfo()->Effect[0] == SPELL_EFFECT_SKINNING)  // skinning
            {
                // if target doesn't have skinnable flag, don't let it be skinned
                if (!target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE))
                    return SPELL_FAILED_TARGET_UNSKINNABLE;
                // if target is already skinned, don't let it be skinned again
                if (target->IsCreature() && static_cast<Creature*>(target)->Skinned)
                    return SPELL_FAILED_TARGET_UNSKINNABLE;
            }

            // all spells with target 61 need to be in group or raid
            ///\todo need to research this if its not handled by the client!!!
            if (GetSpellInfo()->EffectImplicitTargetA[0] == EFF_TARGET_AREAEFFECT_PARTY_AND_CLASS ||
                GetSpellInfo()->EffectImplicitTargetA[1] == EFF_TARGET_AREAEFFECT_PARTY_AND_CLASS ||
                GetSpellInfo()->EffectImplicitTargetA[2] == EFF_TARGET_AREAEFFECT_PARTY_AND_CLASS)
            {
                if (target->IsPlayer() && !static_cast<Player*>(target)->InGroup())
                    return SPELL_FAILED_TARGET_NOT_IN_PARTY;
            }

            // fishing spells
            if (GetSpellInfo()->EffectImplicitTargetA[0] == EFF_TARGET_SELF_FISHING)  //||
                                                                                   //GetProto()->EffectImplicitTargetA[1] == EFF_TARGET_SELF_FISHING ||
                                                                                   //GetProto()->EffectImplicitTargetA[2] == EFF_TARGET_SELF_FISHING)
            {
                uint32 entry = GetSpellInfo()->EffectMiscValue[0];
                if (entry == GO_FISHING_BOBBER)
                {
                    //uint32 mapid = p_caster->GetMapId();
                    float px = u_caster->GetPositionX();
                    float py = u_caster->GetPositionY();
                    float pz = u_caster->GetPositionZ();
                    float orient = m_caster->GetOrientation();
                    float posx = 0, posy = 0, posz = 0;
                    float co = cos(orient);
                    float si = sin(orient);
                    MapMgr* map = m_caster->GetMapMgr();

                    float r;
                    for (r = 20; r > 10; r--)
                    {
                        posx = px + r * co;
                        posy = py + r * si;
                        uint32 liquidtype;
                        map->GetLiquidInfo(posx, posy, pz + 2, posz, liquidtype);
                        if (!(liquidtype & 1))//water
                            continue;
                        if (!map->isInLineOfSight(px, py, pz + 0.5f, posx, posy, posz))
                            continue;
                        if (posz > map->GetLandHeight(posx, posy, pz + 2))
                            break;
                    }
                    if (r <= 10)
                        return SPELL_FAILED_NOT_FISHABLE;

                    // if we are already fishing, don't cast it again
                    if (p_caster->GetSummonedObject())
                        if (p_caster->GetSummonedObject()->GetEntry() == GO_FISHING_BOBBER)
                            return SPELL_FAILED_SPELL_IN_PROGRESS;
                }
            }

            if (p_caster != NULL)
            {
                if (GetSpellInfo()->custom_NameHash == SPELL_HASH_GOUGE)  // Gouge
                    if (!target->isInFront(p_caster))
                        return SPELL_FAILED_NOT_INFRONT;

                if (GetSpellInfo()->Category == 1131) //Hammer of wrath, requires target to have 20- % of hp
                {
                    if (target->GetHealth() == 0)
                        return SPELL_FAILED_BAD_TARGETS;

                    if (target->GetMaxHealth() / target->GetHealth() < 5)
                        return SPELL_FAILED_BAD_TARGETS;
                }
                else if (GetSpellInfo()->Category == 672) //Conflagrate, requires immolation spell on victim
                {
                    if (!target->HasAuraVisual(46))
                        return SPELL_FAILED_BAD_TARGETS;
                }

                if (target->dispels[GetSpellInfo()->DispelType])
                    return SPELL_FAILED_DAMAGE_IMMUNE;			// hackfix - burlex

                                                                // Removed by Supalosa and moved to 'completed cast'
                                                                //if (target->MechanicsDispels[GetProto()->MechanicsType])
                                                                //	return SPELL_FAILED_PREVENTED_BY_MECHANIC-1; // Why not just use 	SPELL_FAILED_DAMAGE_IMMUNE                                   = 144,
            }

            // if we're replacing a higher rank, deny it
            AuraCheckResponse acr = target->AuraCheck(GetSpellInfo(), m_caster);
            if (acr.Error == AURA_CHECK_RESULT_HIGHER_BUFF_PRESENT)
                return SPELL_FAILED_AURA_BOUNCED;

            //check if we are trying to stealth or turn invisible but it is not allowed right now
            if (IsStealthSpell() || IsInvisibilitySpell())
            {
                //if we have Faerie Fire, we cannot stealth or turn invisible
                if (u_caster->FindAuraByNameHash(SPELL_HASH_FAERIE_FIRE) || u_caster->FindAuraByNameHash(SPELL_HASH_FAERIE_FIRE__FERAL_))
                    return SPELL_FAILED_SPELL_UNAVAILABLE;
            }
        }
    }

    // Special State Checks (for creatures & players)
    if (u_caster != NULL)
    {
        if (u_caster->SchoolCastPrevent[GetSpellInfo()->School])
        {
            uint32 now_ = getMSTime();
            if (now_ > u_caster->SchoolCastPrevent[GetSpellInfo()->School]) //this limit has expired,remove
                u_caster->SchoolCastPrevent[GetSpellInfo()->School] = 0;
            else
            {
                // HACK FIX
                switch (GetSpellInfo()->custom_NameHash)
                {
                    // This is actually incorrect. school lockouts take precedence over silence.
                    // So ice block/divine shield are not usable while their schools are locked out,
                    // but can be used while silenced.
                    //case SPELL_HASH_ICE_BLOCK: //Ice Block
                    //case 0x9840A1A6: //Divine Shield
                    //break;
                    
                case SPELL_HASH_WILL_OF_THE_FORSAKEN:
                {
                    if (u_caster->m_special_state & (UNIT_STATE_FEAR | UNIT_STATE_CHARM | UNIT_STATE_SLEEP))
                        break;
                }
                break;

                case SPELL_HASH_DEATH_WISH:
                case SPELL_HASH_FEAR_WARD:
                case SPELL_HASH_BERSERKER_RAGE:
                {
                    if (u_caster->m_special_state & UNIT_STATE_FEAR)
                        break;
                }
                break;

                // {Insignia|Medallion} of the {Horde|Alliance}
                case SPELL_HASH_PVP_TRINKET:
                case SPELL_HASH_EVERY_MAN_FOR_HIMSELF:
                case SPELL_HASH_DIVINE_SHIELD:
                {
                    if (u_caster->m_special_state & (UNIT_STATE_FEAR | UNIT_STATE_CHARM | UNIT_STATE_SLEEP | UNIT_STATE_ROOT | UNIT_STATE_STUN | UNIT_STATE_CONFUSE | UNIT_STATE_SNARE))
                        break;
                }
                break;

                case SPELL_HASH_BARKSKIN:
                {
                    // This spell is usable while stunned, frozen, incapacitated, feared or asleep.  Lasts 12 sec.
                    if (u_caster->m_special_state & (UNIT_STATE_STUN | UNIT_STATE_FEAR | UNIT_STATE_SLEEP))     // Uh, what unit_state is Frozen? (freezing trap...)
                        break;
                }
                break;

                case SPELL_HASH_DISPERSION:
                {
                    if (u_caster->m_special_state & (UNIT_STATE_FEAR | UNIT_STATE_STUN | UNIT_STATE_SILENCE))
                        break;
                }
                break;

                default:
                    return SPELL_FAILED_SILENCED;
                }
            }
        }

        // can only silence non-physical
        if (u_caster->m_silenced && GetSpellInfo()->School != SCHOOL_NORMAL)
        {
            switch (GetSpellInfo()->custom_NameHash)
            {
            case SPELL_HASH_ICE_BLOCK: //Ice Block
            case SPELL_HASH_DIVINE_SHIELD: //Divine Shield
            case SPELL_HASH_DISPERSION:
                break;

            default:
                return SPELL_FAILED_SILENCED;
            }
        }

        Unit* target = (m_caster->IsInWorld()) ? m_caster->GetMapMgr()->GetUnit(m_targets.m_unitTarget) : NULL;
        if (target)  // -Supalosa- Shouldn't this be handled on Spell Apply? 
        {
            for (i = 0; i < 3; i++)  // if is going to cast a spell that breaks stun remove stun auras, looks a bit hacky but is the best way i can find
            {
                if (GetSpellInfo()->EffectApplyAuraName[i] == SPELL_AURA_MECHANIC_IMMUNITY)
                {
                    target->RemoveAllAurasByMechanic(GetSpellInfo()->EffectMiscValue[i], static_cast<uint32>(-1), true);
                    // Remove all debuffs of that mechanic type.
                    // This is also done in SpellAuras.cpp - wtf?
                }
            }
        }

        // only affects physical damage
        if (u_caster->IsPacified() && GetSpellInfo()->School == SCHOOL_NORMAL)
        {
            // HACK FIX
            switch (GetSpellInfo()->custom_NameHash)
            {
            case SPELL_HASH_ICE_BLOCK: //Ice Block
            case SPELL_HASH_DIVINE_SHIELD: //Divine Shield
            case SPELL_HASH_WILL_OF_THE_FORSAKEN: //Will of the Forsaken
            case SPELL_HASH_EVERY_MAN_FOR_HIMSELF: // Every Man for Himself
            {
                if (u_caster->m_special_state & (UNIT_STATE_FEAR | UNIT_STATE_CHARM | UNIT_STATE_SLEEP))
                    break;
            }
            break;

            default:
                return SPELL_FAILED_PACIFIED;
            }
        }


        if (u_caster->GetChannelSpellTargetGUID() != 0)
        {
            SpellInfo const* t_spellInfo = (u_caster->GetCurrentSpell() ? u_caster->GetCurrentSpell()->GetSpellInfo() : NULL);

            if (!t_spellInfo || !m_triggeredSpell)
                return SPELL_FAILED_SPELL_IN_PROGRESS;
            else if (t_spellInfo)
            {
                if (t_spellInfo->EffectTriggerSpell[0] != GetSpellInfo()->Id &&
                    t_spellInfo->EffectTriggerSpell[1] != GetSpellInfo()->Id &&
                    t_spellInfo->EffectTriggerSpell[2] != GetSpellInfo()->Id)
                {
                    return SPELL_FAILED_SPELL_IN_PROGRESS;
                }
            }
        }
    }

    //Dead pet check
    if (GetSpellInfo()->AttributesExB & ATTRIBUTESEXB_REQ_DEAD_PET && p_caster != NULL)
    {
        Pet* pPet = p_caster->GetSummon();
        if (pPet != NULL && !pPet->IsDead())
            return SPELL_FAILED_TARGET_NOT_DEAD;
    }

    // no problems found, so we must be ok
    return SPELL_CANCAST_OK;*/
}

SpellCastResult Spell::getErrorAtShapeshiftedCast(SpellInfo const* spellInfo, const uint32_t form) const
{
    // from MaNGOS
    // "talents that learn spells can have stance requirements that need ignore
    // (this requirement only for client-side stance show in talent description)"
    uint32_t talentRank = 0;
    if (auto talentInfo = sTalentStore.LookupEntry(spellInfo->Id))
    {
        for (int i = 0; i < 5; ++i)
        {
            if (talentInfo->RankID[i])
                talentRank = i + 1;
        }
    }

    if (talentRank > 0 && spellInfo->HasEffect(SPELL_EFFECT_LEARN_SPELL))
        return SPELL_FAILED_SUCCESS;

    const uint32_t stanceMask = (form ? 1 << (form - 1) : 0);

    if (stanceMask & spellInfo->ShapeshiftExclude) // can explicitly not be casted in this stance
        return SPELL_FAILED_NOT_SHAPESHIFT;

    if (stanceMask & spellInfo->RequiredShapeShift) // can explicitly be casted in this stance
        return SPELL_FAILED_SUCCESS;

    bool actAsShifted = false;
    if (form > FORM_NORMAL)
    {
        auto shapeshift_form = sSpellShapeshiftFormStore.LookupEntry(form);
        if (!shapeshift_form)
        {
            LOG_ERROR("Spell::getErrorAtShapeshiftedCast: caster has unknown shapeshift form %u", form);
            return SPELL_FAILED_SUCCESS;
        }
        actAsShifted = !(shapeshift_form->Flags & 1); // shapeshift acts as normal form for spells
    }

    if (actAsShifted)
    {
        if (GetSpellInfo()->hasAttributes(ATTRIBUTES_NOT_SHAPESHIFT)) // not while shapeshifted
            return SPELL_FAILED_NOT_SHAPESHIFT;
        else if (spellInfo->RequiredShapeShift != 0) // needs other shapeshift
            return SPELL_FAILED_ONLY_SHAPESHIFT;
    }
    else
    {
        // Check if it even requires a shapeshift....
        if (!GetSpellInfo()->hasAttributes(ATTRIBUTESEXB_NOT_NEED_SHAPESHIFT) && spellInfo->RequiredShapeShift != 0)
            return SPELL_FAILED_ONLY_SHAPESHIFT;
    }
    return SPELL_FAILED_SUCCESS;
}
#endif
