/*
 * AscEmu Framework based on ArcEmu MMORPG Server
 * Copyright (c) 2014-2020 AscEmu Team <http://www.ascemu.org>
 * Copyright (C) 2008-2012 ArcEmu Team <http://www.ArcEmu.org/>
 * Copyright (C) 2005-2007 Ascent Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "StdAfx.h"
#include "Units/Unit.h"
#include "Storage/DBC/DBCStores.h"
#include "Management/QuestLogEntry.hpp"
#include "Server/EventableObject.h"
#include "Server/IUpdatable.h"
#include "VMapFactory.h"
#include "MMapFactory.h"
#include "TLSObject.h"
#include "Management/Battleground/Battleground.h"
#include "Management/ItemInterface.h"
#include "Server/WorldSocket.h"
#include "Storage/MySQLDataStore.hpp"
#include "Map/MapMgrDefines.hpp"
#include "Map/Area/AreaStorage.hpp"
#include "Map/MapMgr.h"
#include "Faction.h"
#include "Map/WorldCreator.h"
#include "Spell/Definitions/ProcFlags.h"
#include "Spell/Definitions/SpellDamageType.h"
#include "Spell/Definitions/SpellMechanics.h"
#include "Spell/Definitions/SpellState.h"
#include <Spell/Definitions/AuraInterruptFlags.h>
#include "Spell/Definitions/SpellSchoolConversionTable.h"
#include "Spell/Definitions/PowerType.h"
#include "Spell/SpellMgr.h"
#include "Units/Creatures/CreatureDefines.hpp"
#include "Data/WoWObject.h"
#include "Data/WoWPlayer.h"
#include "Data/WoWGameObject.h"
#include "Server/Packets/SmsgDestoyObject.h"
#include "Server/Packets/SmsgPlaySound.h"
#include "Server/Packets/SmsgGameobjectDespawnAnim.h"
#include "Server/Packets/SmsgSpellLogMiss.h"
#include "Server/Packets/SmsgAiReaction.h"
#include "Server/OpcodeTable.hpp"

// MIT Start

using namespace AscEmu::Packets;

bool Object::write(const uint8_t& member, uint8_t val)
{
    if (member == val)
        return false;

    const auto member_ptr = const_cast<uint8_t*>(&member);
    *member_ptr = val;

    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance -= distance % 4;
    distance /= 4;

    m_updateMask.SetBit(distance);

    if (!skipping_updates)
        updateObject();

    return true;
}

bool Object::write(const uint16_t& member, uint16_t val)
{
    if (member == val)
        return false;

    const auto nonconst_member = const_cast<uint16_t*>(&member);
    *nonconst_member = val;

    const auto member_ptr = reinterpret_cast<uint8_t*>(nonconst_member);
    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance /= 4;

    m_updateMask.SetBit(distance);
    m_updateMask.SetBit(distance + 1);

    if (!skipping_updates)
        updateObject();

    return true;
}

bool Object::write(const float& member, float val)
{
    if (member == val)
        return false;

    const auto nonconst_member = const_cast<float*>(&member);
    *nonconst_member = val;

    const auto member_ptr = reinterpret_cast<uint8_t*>(nonconst_member);
    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance /= 4;

    m_updateMask.SetBit(distance);

    if (!skipping_updates)
        updateObject();

    return true;
}

bool Object::write(const int32_t& member, int32_t val)
{
    if (member == val)
        return false;

    const auto nonconst_member = const_cast<int32_t*>(&member);
    *nonconst_member = val;

    const auto member_ptr = reinterpret_cast<uint8_t*>(nonconst_member);
    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance /= 4;

    m_updateMask.SetBit(distance);

    if (!skipping_updates)
        updateObject();

    return true;
}

bool Object::write(const uint32_t& member, uint32_t val)
{
    if (member == val)
        return false;

    const auto nonconst_member = const_cast<uint32_t*>(&member);
    *nonconst_member = val;

    const auto member_ptr = reinterpret_cast<uint8_t*>(nonconst_member);
    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance /= 4;

    m_updateMask.SetBit(distance);

    if (!skipping_updates)
        updateObject();

    return true;
}

bool Object::write(const uint64_t& member, uint64_t val)
{
    if (member == val)
        return false;

    const auto nonconst_member = const_cast<uint64_t*>(&member);
    *nonconst_member = val;

    const auto member_ptr = reinterpret_cast<uint8_t*>(nonconst_member);
    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance /= 4;

    m_updateMask.SetBit(distance);
    m_updateMask.SetBit(distance + 1);

    if (!skipping_updates)
        updateObject();

    return true;
}

bool Object::writeLow(const uint64_t& member, uint32_t val)
{
    if (member == val)
        return false;

    const auto nonconst_member = const_cast<uint64_t*>(&member);
    *reinterpret_cast<uint32_t*>(*nonconst_member) = val;

    const auto member_ptr = reinterpret_cast<uint8_t*>(nonconst_member);
    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance /= 4;

    m_updateMask.SetBit(distance);

    if (!skipping_updates)
        updateObject();

    return true;
}

bool Object::writeHigh(const uint64_t& member, uint32_t val)
{
    if (member == val)
        return false;

    const auto nonconst_member = const_cast<uint64_t*>(&member);
    *(reinterpret_cast<uint32_t*>(*nonconst_member) + 1) = val;

    const auto member_ptr = reinterpret_cast<uint8_t*>(nonconst_member);
    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance /= 4;

    m_updateMask.SetBit(distance + 1);

    if (!skipping_updates)
        updateObject();

    return true;
}

bool Object::write(const uint64_t& member, uint32_t low, uint32_t high)
{
    const auto nonconst_member = const_cast<uint64_t*>(&member);
    const auto low_ptr = reinterpret_cast<uint32_t*>(*nonconst_member);
    const auto high_ptr = low_ptr + 1;

    if (*low_ptr == low && *high_ptr == high)
        return false;

    *low_ptr = low;
    *high_ptr = high;

    const auto member_ptr = reinterpret_cast<uint8_t*>(nonconst_member);
    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance /= 4;

    m_updateMask.SetBit(distance);
    m_updateMask.SetBit(distance + 1);

    if (!skipping_updates)
        updateObject();

    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// WoWData
uint64_t Object::getGuid() const { return objectData()->guid; }
void Object::setGuid(uint64_t guid)
{
    write(objectData()->guid, guid);
    m_wowGuid.Init(guid);
}
void Object::setGuid(uint32_t low, uint32_t high) { setGuid(static_cast<uint64_t>(high) << 32 | low); }

uint32_t Object::getGuidLow() const { return objectData()->guid_parts.low; }
void Object::setGuidLow(uint32_t low) { setGuid(low, objectData()->guid_parts.high); }

uint32_t Object::getGuidHigh() const { return objectData()->guid_parts.high; }
void Object::setGuidHigh(uint32_t high) { setGuid(objectData()->guid_parts.low, high); }

#if VERSION_STRING < Cata
uint32_t Object::getOType() const { return objectData()->type; }
void Object::setOType(uint32_t type) { write(objectData()->type, type); }
void Object::setObjectType(uint8_t objectTypeId)
{
    uint16_t object_type = TYPE_OBJECT;
    switch (objectTypeId)
    {
    case TYPEID_CONTAINER:
        object_type |= TYPE_CONTAINER;
    case TYPEID_ITEM:
        object_type |= TYPE_ITEM;
        break;
    case TYPEID_PLAYER:
        object_type |= TYPE_PLAYER;
    case TYPEID_UNIT:
        object_type |= TYPE_UNIT;
        break;
    case TYPEID_GAMEOBJECT:
        object_type |= TYPE_GAMEOBJECT;
        break;
    case TYPEID_DYNAMICOBJECT:
        object_type |= TYPE_DYNAMICOBJECT;
        break;
    case TYPEID_CORPSE:
        object_type |= TYPE_CORPSE;
        break;
    default:
        break;
    }

    m_objectType = object_type;
    m_objectTypeId = objectTypeId;
    write(objectData()->type, static_cast<uint32_t>(m_objectType));
}
#else
uint16_t Object::getOType() const { return objectData()->parts.type; }
void Object::setOType(uint16_t type) { write(objectData()->parts.type, type); }
void Object::setObjectType(uint8_t objectTypeId)
{
    uint16_t object_type = TYPE_OBJECT;
    switch (objectTypeId)
    {
    case TYPEID_CONTAINER:
        object_type |= TYPE_CONTAINER;
    case TYPEID_ITEM:
        object_type |= TYPE_ITEM;
        break;
    case TYPEID_PLAYER:
        object_type |= TYPE_PLAYER;
    case TYPEID_UNIT:
        object_type |= TYPE_UNIT;
        break;
    case TYPEID_GAMEOBJECT:
        object_type |= TYPE_GAMEOBJECT;
        break;
    case TYPEID_DYNAMICOBJECT:
        object_type |= TYPE_DYNAMICOBJECT;
        break;
    case TYPEID_CORPSE:
        object_type |= TYPE_CORPSE;
        break;
    default:
        break;
    }

    m_objectType = object_type;
    m_objectTypeId = objectTypeId;
    write(objectData()->parts.type, static_cast<uint16_t>(m_objectType));
}
#endif

uint32_t Object::getEntry() const { return objectData()->entry; }
void Object::setEntry(uint32_t entry) { write(objectData()->entry, entry); }

float Object::getScale() const { return objectData()->scale_x; }
void Object::setScale(float scaleX) { write(objectData()->scale_x, scaleX); }

//////////////////////////////////////////////////////////////////////////////////////////
// Object update
void Object::updateObject()
{
    if (IsInWorld() && !m_objectUpdated)
    {
        m_mapMgr->ObjectUpdated(this);
        m_objectUpdated = true;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// Object Type Id
uint8_t Object::getObjectTypeId() const { return m_objectTypeId; }

bool Object::isCreatureOrPlayer() const { return (getObjectTypeId() == TYPEID_UNIT || getObjectTypeId() == TYPEID_PLAYER); }
bool Object::isPlayer() const { return getObjectTypeId() == TYPEID_PLAYER; }
bool Object::isCreature() const { return getObjectTypeId() == TYPEID_UNIT; }
bool Object::isItem() const { return getObjectTypeId() == TYPEID_ITEM; }
bool Object::isGameObject() const { return getObjectTypeId() == TYPEID_GAMEOBJECT; }
bool Object::isCorpse() const { return getObjectTypeId() == TYPEID_CORPSE; }
bool Object::isContainer() const { return getObjectTypeId() == TYPEID_CONTAINER; }

//////////////////////////////////////////////////////////////////////////////////////////
// Position functions
bool Object::isInRange(LocationVector location, float square_r) const
{
    return getDistanceSq(location) <= square_r;
}

bool Object::isInRange(float x, float y, float z, float square_r) const
{
    return getDistanceSq(x, y, z) <= square_r;
}

float Object::getDistanceSq(LocationVector comp) const
{
    return comp.distanceSquare(m_position);
}

float Object::getDistanceSq(float x, float y, float z) const
{
    return m_position.distanceSquare({ x, y, z });
}

Player* Object::asPlayer()
{
    return reinterpret_cast<Player*>(this);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Spell functions
Spell* Object::getCurrentSpell(CurrentSpellType spellType) const
{
    return m_currentSpell[spellType];
}

Spell* Object::getCurrentSpellById(uint32_t spellId) const
{
    for (uint8_t i = 0; i < CURRENT_SPELL_MAX; ++i)
    {
        if (m_currentSpell[i] == nullptr)
            continue;
        if (m_currentSpell[i]->getSpellInfo()->getId() == spellId)
            return m_currentSpell[i];
    }
    return nullptr;
}

void Object::setCurrentSpell(Spell* curSpell)
{
    ARCEMU_ASSERT(curSpell != nullptr); // curSpell cannot be nullptr

    // Get current spell type
    CurrentSpellType spellType = CURRENT_GENERIC_SPELL;
    if (curSpell->getSpellInfo()->isOnNextMeleeAttack())
    {
        // Melee spell
        spellType = CURRENT_MELEE_SPELL;
    }
    else if (curSpell->getSpellInfo()->isRangedAutoRepeat())
    {
        // Autorepeat spells (Auto shot / Shoot (wand))
        spellType = CURRENT_AUTOREPEAT_SPELL;
    }
    else if (curSpell->getSpellInfo()->isChanneled())
    {
        // Channeled spells
        spellType = CURRENT_CHANNELED_SPELL;
    }

    // We've already set this spell to current spell, ignore
    if (curSpell == m_currentSpell[spellType])
        return;

    // Interrupt spell with same spell type
    interruptSpellWithSpellType(spellType);

    // Handle spelltype specific cases
    switch (spellType)
    {
        case CURRENT_GENERIC_SPELL:
        {
            // Generic spells break channeled spells
            interruptSpellWithSpellType(CURRENT_CHANNELED_SPELL);

            if (m_currentSpell[CURRENT_AUTOREPEAT_SPELL] != nullptr)
            {
                // Generic spells do not break Auto Shot
                if (m_currentSpell[CURRENT_AUTOREPEAT_SPELL]->getSpellInfo()->getId() != 75)
                    interruptSpellWithSpellType(CURRENT_AUTOREPEAT_SPELL);
            }
            break;
        }
        case CURRENT_CHANNELED_SPELL:
        {
            // Channeled spells break generic spells
            interruptSpellWithSpellType(CURRENT_GENERIC_SPELL);
            // as well break channeled spells too
            interruptSpellWithSpellType(CURRENT_CHANNELED_SPELL);

            // Also break autorepeat spells, unless it's Auto Shot
            if (m_currentSpell[CURRENT_AUTOREPEAT_SPELL] != nullptr && m_currentSpell[CURRENT_AUTOREPEAT_SPELL]->getSpellInfo()->getId() != 75)
            {
                interruptSpellWithSpellType(CURRENT_AUTOREPEAT_SPELL);
            }
            break;
        }
        case CURRENT_AUTOREPEAT_SPELL:
        {
            // Other autorepeats than Auto Shot break non-delayed generic and channeled spells
            if (curSpell->getSpellInfo()->getId() != 75)
            {
                interruptSpellWithSpellType(CURRENT_GENERIC_SPELL);
                interruptSpellWithSpellType(CURRENT_CHANNELED_SPELL);
            }

            if (isPlayer())
            {
                static_cast<Player*>(this)->m_FirstCastAutoRepeat = true;
            }
            break;
        }
        default:
            break;
    }

    // If spell is not yet cancelled, force it
    if (m_currentSpell[spellType] != nullptr)
    {
        m_currentSpell[spellType]->finish(false);
    }

    // Set new current spell
    m_currentSpell[spellType] = curSpell;
}

void Object::interruptSpell(uint32_t spellId, bool checkMeleeSpell)
{
    for (uint8_t i = 0; i < CURRENT_SPELL_MAX; ++i)
    {
        if (!checkMeleeSpell && i == CURRENT_MELEE_SPELL)
            continue;

        if (m_currentSpell[i] != nullptr &&
            (spellId == 0 || m_currentSpell[i]->getSpellInfo()->getId() == spellId))
        {
            interruptSpellWithSpellType(CurrentSpellType(i));
        }
    }
}

void Object::interruptSpellWithSpellType(CurrentSpellType spellType)
{
    Spell* curSpell = m_currentSpell[spellType];
    if (curSpell != nullptr)
    {
        if (spellType == CURRENT_AUTOREPEAT_SPELL)
        {
            if (isPlayer() && IsInWorld())
            {
                // Send server-side cancel message
                auto spellId = curSpell->getSpellInfo()->getId();
                //\Note: seems to be wrong format for this opcode - guess guid is required!
                //static_cast<Player*>(this)->OutPacket(SMSG_CANCEL_AUTO_REPEAT, 4, &spellId);
            }
        }

        // Cancel spell
        curSpell->cancel();
        m_currentSpell[spellType] = nullptr;
    }
}

bool Object::isCastingSpell(bool skipChanneled /*= false*/, bool skipAutorepeat /*= false*/, bool isAutoshoot /*= false*/) const
{
    // Check generic spell, but ignore finished spells
    if (m_currentSpell[CURRENT_GENERIC_SPELL] != nullptr && m_currentSpell[CURRENT_GENERIC_SPELL]->getState() != SPELL_STATE_FINISHED && m_currentSpell[CURRENT_GENERIC_SPELL]->getCastTimeLeft() > 0 &&
        (!isAutoshoot || !(m_currentSpell[CURRENT_GENERIC_SPELL]->getSpellInfo()->getAttributesExB() & ATTRIBUTESEXB_NOT_RESET_AUTO_ATTACKS)))
    {
        return true;
    }

    // If not skipped, check channeled spell
    if (!skipChanneled && m_currentSpell[CURRENT_CHANNELED_SPELL] != nullptr && m_currentSpell[CURRENT_CHANNELED_SPELL]->getState() != SPELL_STATE_FINISHED &&
        (!isAutoshoot || !(m_currentSpell[CURRENT_CHANNELED_SPELL]->getSpellInfo()->getAttributesExB() & ATTRIBUTESEXB_NOT_RESET_AUTO_ATTACKS)))
    {
        return true;
    }

    // If not skipped, check autorepeat spell
    if (!skipAutorepeat && m_currentSpell[CURRENT_AUTOREPEAT_SPELL] != nullptr)
    {
        return true;
    }
    return false;
}

Spell* Object::findCurrentCastedSpellBySpellId(uint32_t spellId)
{
    for (uint8_t i = 0; i < CURRENT_SPELL_MAX; ++i)
    {
        if (m_currentSpell[i] == nullptr)
            continue;
        if (m_currentSpell[i]->getSpellInfo()->getId() == spellId)
            return m_currentSpell[i];
    }
    return nullptr;
}

int32_t Object::doSpellDamage(Unit* victim, uint32_t spellId, int32_t dmg, uint8_t effIndex, bool allowProc/* = true*/, bool staticDamage/* = false*/, bool isPeriodic/* = false*/, bool isLeech/* = false*/, bool forceCrit/* = false*/, Aura* aur/* = nullptr*/, AuraEffectModifier* aurEff/* = nullptr*/)
{
    if (victim == nullptr || !victim->isAlive())
        return 0;

    const auto spellInfo = sSpellMgr.getSpellInfo(spellId);
    if (spellInfo == nullptr)
        return 0;

    float_t damage = static_cast<float_t>(dmg);
    const auto school = spellInfo->getFirstSchoolFromSchoolMask();
    auto isCritical = forceCrit;

    // Check if victim is immune to this school
    // or if victim has god mode cheat
    if (victim->SchoolImmunityList[school] != 0 ||
        (victim->isPlayer() && static_cast<Player*>(victim)->m_cheats.GodModeCheat))
    {
        if (isCreatureOrPlayer())
            static_cast<Unit*>(this)->sendSpellOrDamageImmune(getGuid(), victim, spellId);

        return 0;
    }

    // Setup proc flags
    uint32_t casterProcFlags = PROC_ON_ANY_HOSTILE_ACTION;
    uint32_t victimProcFlags = PROC_ON_ANY_HOSTILE_ACTION | PROC_ON_ANY_DAMAGE_VICTIM;

    if (!isPeriodic)
    {
        switch (spellInfo->getDmgClass())
        {
            case SPELL_DMG_TYPE_MAGIC:
                casterProcFlags |= PROC_ON_SPELL_HIT;
                victimProcFlags |= PROC_ON_SPELL_HIT_VICTIM;
                break;
            case SPELL_DMG_TYPE_MELEE:
                casterProcFlags |= PROC_ON_MELEE_ATTACK;
                victimProcFlags |= PROC_ON_MELEE_ATTACK_VICTIM;
                break;
            case SPELL_DMG_TYPE_RANGED:
                casterProcFlags |= PROC_ON_RANGED_ATTACK;
                victimProcFlags |= PROC_ON_RANGED_ATTACK_VICTIM;
                break;
            default:
                break;
        }
    }

    // Get spell damage bonus
    if (isCreatureOrPlayer() && !staticDamage)
    {
        const auto casterUnit = static_cast<Unit*>(this);
        casterUnit->RemoveAurasByInterruptFlag(AURA_INTERRUPT_ON_START_ATTACK);

        auto bonusDamage = casterUnit->getSpellDamageBonus(spellInfo, dmg, isPeriodic, aur);
        if (isPeriodic && aur != nullptr)
            bonusDamage *= aur->getStackCount();

        damage += bonusDamage;
        if (damage < 0.0f)
            damage = 0.0f;
    }

    // Mage talent - Torment the Weak
    if (isPlayer() && (victim->HasAuraWithMechanics(MECHANIC_ENSNARED) || victim->HasAuraWithMechanics(MECHANIC_DAZED)))
    {
        const auto pct = static_cast<float_t>(static_cast<Player*>(this)->m_IncreaseDmgSnaredSlowed);
        damage = damage * (1.0f + (pct / 100.0f));
    }

    // Add creature type bonuses
    if (isPlayer() && victim->isCreature())
    {
        const auto plr = static_cast<Player*>(this);
        const auto cre = static_cast<Creature*>(victim);
        const auto type = cre->GetCreatureProperties()->Type;

        damage += damage * plr->IncreaseDamageByTypePCT[type];
        if (isPeriodic)
        {
            if (aur != nullptr && aurEff != nullptr)
                damage += static_cast<float_t>(plr->IncreaseDamageByType[type] / (aur->getMaxDuration() / aurEff->mAmplitude));
        }
        else
        {
            damage += static_cast<float_t>(plr->IncreaseDamageByType[type]);
        }
    }

    // Check if spell can crit
    if (damage > 0.0f)
    {
        // If the damage is forced to be crit, do not alter it
        if (!isCritical && !(spellInfo->getAttributesExB() & ATTRIBUTESEXB_CANT_CRIT))
        {
            if (isPeriodic)
            {
                // For periodic effects use aura's own crit chance
                if (aur != nullptr && aur->getCritChance() > 0.0f)
                    isCritical = Rand(aur->getCritChance());
            }
            else
            {
                isCritical = IsCriticalDamageForSpell(victim, spellInfo);
            }
        }

        // Get critical spell damage bonus
        if (isCritical)
        {
            damage = GetCriticalDamageBonusForSpell(victim, spellInfo, damage);

            switch (spellInfo->getDmgClass())
            {
                case SPELL_DMG_TYPE_MAGIC:
                    casterProcFlags |= PROC_ON_SPELL_CRIT_HIT;
                    victimProcFlags |= PROC_ON_SPELL_CRIT_HIT_VICTIM;
                    break;
                case SPELL_DMG_TYPE_MELEE:
                    casterProcFlags |= PROC_ON_CRIT_ATTACK;
                    victimProcFlags |= PROC_ON_CRIT_HIT_VICTIM;
                    break;
                case SPELL_DMG_TYPE_RANGED:
                    casterProcFlags |= PROC_ON_RANGED_CRIT_ATTACK;
                    victimProcFlags |= PROC_ON_RANGED_CRIT_ATTACK_VICTIM;
                    break;
                default:
                    break;
            }
        }
    }

    // Calculate damage reduction
    damage += damage * victim->DamageTakenPctMod[school];
    damage += damage * victim->ModDamageTakenByMechPCT[spellInfo->getMechanicsType()];
    if (isPeriodic)
    {
        if (aur != nullptr && aurEff != nullptr)
            damage += static_cast<float_t>(victim->DamageTakenMod[school] / (aur->getMaxDuration() / aurEff->mAmplitude));
    }
    else
    {
        damage += victim->DamageTakenMod[school];
    }

    // Resilience
    ///\ todo: move this elsewhere and fix this
    float_t damageReductionPct = 1.0f;
    if (victim->isPlayer())
    {
        auto resilienceValue = static_cast<float_t>(static_cast<Player*>(victim)->CalcRating(PCR_SPELL_CRIT_RESILIENCE) / 100.0f);
        if (resilienceValue > 1.0f)
            resilienceValue = 1.0f;
        damageReductionPct -= resilienceValue;
    }
    damage *= damageReductionPct;

    if (damage < 0.0f)
        damage = 0.0f;

    uint32_t damage_int = 0;
    // For periodic auras convert the float amount to integer and save the damage fraction for next tick
    if (isPeriodic && aur != nullptr && aurEff != nullptr)
    {
        damage += aurEff->mDamageFraction;
        if (aur->getTimeLeft() >= aurEff->mAmplitude)
        {
            damage_int = static_cast<uint32_t>(damage);
            aurEff->mDamageFraction = damage - damage_int;
        }
        else
        {
            // In case this is the last tick, just round the value
            damage_int = static_cast<uint32_t>(std::round(damage));
        }
    }
    else
    {
        // If this is a direct damage spell just round the value
        damage_int = static_cast<uint32_t>(std::round(damage));
    }

    // Check for absorb effects
    auto absorbedDamage = victim->AbsorbDamage(school, &damage_int);
    const auto manaShieldAbsorb = victim->ManaShieldAbsorb(damage_int);

    if (manaShieldAbsorb > 0)
    {
        if (manaShieldAbsorb > damage_int)
            damage_int = 0;
        else
            damage_int -= manaShieldAbsorb;

        absorbedDamage += manaShieldAbsorb;
    }

    if (absorbedDamage > 0)
        victimProcFlags |= PROC_ON_ABSORB;

    // Hackfix from legacy method
    // Incanter's Absorption
    if (victim->isPlayer())
    {
        uint32 incanterSAbsorption[] =
        {
            //SPELL_HASH_INCANTER_S_ABSORPTION
            44394,
            44395,
            44396,
            44413,
            0
        };

        if (victim->hasAurasWithId(incanterSAbsorption))
        {
            float_t pctmod = 0.0f;
            const auto pl = static_cast<Player*>(victim);
            if (pl->HasAura(44394))
                pctmod = 0.05f;
            else if (pl->HasAura(44395))
                pctmod = 0.10f;
            else if (pl->HasAura(44396))
                pctmod = 0.15f;

            const auto hp = static_cast<uint32_t>(0.05f * pl->getMaxHealth());
            auto spellpower = static_cast<uint32_t>(pctmod * pl->getModDamageDonePositive(SCHOOL_NORMAL));

            if (spellpower > hp)
                spellpower = hp;

            SpellInfo const* entry = sSpellMgr.getSpellInfo(44413);
            if (entry != nullptr)
                pl->castSpell(pl->getGuid(), entry, spellpower, true);
        }
    }

    // Create damage info
    dealdamage dmginfo;
    dmginfo.school_type = school;
    dmginfo.full_damage = damage_int;
    dmginfo.resisted_damage = 0;

    if (damage_int <= 0)
        dmginfo.resisted_damage = dmginfo.full_damage;

    // Calculate resistance reduction
    if (damage_int > 0 && isCreatureOrPlayer() && !(isPeriodic && school == SCHOOL_NORMAL))
    {
        static_cast<Unit*>(this)->CalculateResistanceReduction(victim, &dmginfo, spellInfo, 0.0f);
        if (dmginfo.resisted_damage > static_cast<uint32_t>(dmginfo.full_damage))
            damage_int = 0;
        else
            damage_int = dmginfo.full_damage - dmginfo.resisted_damage;
    }

    // Check for damage split target
    if (victim->m_damageSplitTarget != nullptr)
        damage_int = victim->DoDamageSplitTarget(damage_int, school, false);

    // Save victim's health before damage is dealt
    const auto healthBefore = victim->getHealth();

    // Send packet and deal damage to victim
    if (isPeriodic && aurEff != nullptr && isCreatureOrPlayer())
    {
        const auto overKill = damage_int > healthBefore ? damage_int - healthBefore : 0;
        const auto wasPacketSent = victim->sendPeriodicAuraLog(GetNewGUID(), victim->GetNewGUID(), spellInfo, damage_int, overKill, absorbedDamage, dmginfo.resisted_damage, aurEff->mAuraEffect, isCritical);

        // In case periodic log couldn't be sent, send normal spell log
        // Required for leech and mana burn auras
        if (!wasPacketSent)
            victim->sendSpellNonMeleeDamageLog(this, victim, spellInfo, damage_int, absorbedDamage, dmginfo.resisted_damage, 0, true, isCritical);
    }
    else
    {
        victim->sendSpellNonMeleeDamageLog(this, victim, spellInfo, damage_int, absorbedDamage, dmginfo.resisted_damage, 0, false, isCritical);
    }

    // Create heal effect for leech effects
    // Must be done before damage is dealt to victim
    if (isCreatureOrPlayer() && isLeech && this != victim)
    {
        // Leech damage can be more than victim's health but the heal should not exceed victim's remaining health
        auto healAmount = std::min(damage_int, healthBefore);

        const uint8_t index = aur != nullptr && aurEff->mAuraEffect != SPELL_AURA_NONE ? aurEff->effIndex : effIndex;
        healAmount = static_cast<uint32_t>(healAmount * spellInfo->getEffectMultipleValue(index));
        doSpellHealing(static_cast<Unit*>(this), spellId, healAmount, true, true, true, true, false, aur, aurEff);
    }

    DealDamage(victim, damage_int, 2, 0, spellId);

    // Handle procs
    if (isCreatureOrPlayer())
    {
        const auto casterUnit = static_cast<Unit*>(this);
        victim->HandleProc(victimProcFlags, casterUnit, spellInfo, !allowProc, damage_int, absorbedDamage);
        victim->m_procCounter = 0;
        casterUnit->HandleProc(casterProcFlags, victim, spellInfo, !allowProc, damage_int, absorbedDamage);
        casterUnit->m_procCounter = 0;
    }

    if (isPlayer())
        static_cast<Player*>(this)->m_casted_amount[school] = damage_int;

    // Cause push back to victim's spell casting if damage was not fully absorbed
    if (!(dmginfo.full_damage == 0 && absorbedDamage > 0))
    {
        if (victim->getCurrentSpell(CURRENT_CHANNELED_SPELL) != nullptr && victim->getCurrentSpell(CURRENT_CHANNELED_SPELL)->getCastTimeLeft() > 0)
            victim->getCurrentSpell(CURRENT_CHANNELED_SPELL)->AddTime(school);
        else if (victim->getCurrentSpell(CURRENT_GENERIC_SPELL) != nullptr && victim->getCurrentSpell(CURRENT_GENERIC_SPELL)->getCastTimeLeft() > 0)
            victim->getCurrentSpell(CURRENT_GENERIC_SPELL)->AddTime(school);
    }

    if (dmginfo.resisted_damage == static_cast<uint32_t>(dmginfo.full_damage) && absorbedDamage == 0)
    {
        // Hackfix from legacy method
        if (victim->isPlayer())
        {
            const auto pVictim = static_cast<Player*>(victim);
            if (pVictim->m_RegenManaOnSpellResist > 0)
            {
                const auto maxMana = pVictim->getMaxPower(POWER_TYPE_MANA);
                const auto amount = static_cast<uint32_t>(maxMana * pVictim->m_RegenManaOnSpellResist);

                pVictim->energize(pVictim, 29442, amount, POWER_TYPE_MANA);
            }

            pVictim->CombatStatusHandler_ResetPvPTimeout();
        }

        if (isPlayer())
            static_cast<Player*>(this)->CombatStatusHandler_ResetPvPTimeout();
    }

    // Hackfix from legacy method
    if (spellInfo->getSchoolMask() & SCHOOL_MASK_SHADOW)
    {
        if (victim->isAlive() && isCreatureOrPlayer())
        {
            //Shadow Word:Death
            if (spellId == 32379 || spellId == 32996 || spellId == 48157 || spellId == 48158)
            {
                auto backfireAmount = damage_int + absorbedDamage;
                const auto absorbed = static_cast<Unit*>(this)->AbsorbDamage(school, &backfireAmount);
                DealDamage(static_cast<Unit*>(this), backfireAmount, 2, 0, spellId);
                static_cast<Unit*>(this)->sendSpellNonMeleeDamageLog(this, this, spellInfo, backfireAmount, absorbed, 0, 0, false, false);
            }
        }
    }

    ///\ todo: at the moment this returns the damage inflicted, but make it return DamageInfo structure
    ///\ required for leech effects, leech should not heal caster if the damage was absorbed
    ///\ but for some spells (i.e. Seed of Corruption) it is mandatory to return the full damage before absorption effects
    return damage_int;
}

void Object::doSpellHealing(Unit* victim, uint32_t spellId, int32_t amt, bool allowProc/* = true*/, bool staticDamage/* = false*/, bool isPeriodic/* = false*/, bool isLeech/* = false*/, bool forceCrit/* = false*/, Aura* aur/* = nullptr*/, AuraEffectModifier* aurEff/* = nullptr*/)
{
    if (victim == nullptr || !victim->isAlive())
        return;

    const auto spellInfo = sSpellMgr.getSpellInfo(spellId);
    if (spellInfo == nullptr)
        return;

    float_t heal = static_cast<float_t>(amt);
    const auto school = spellInfo->getFirstSchoolFromSchoolMask();
    auto isCritical = forceCrit;

    // Setup proc flags
    uint32_t casterProcFlags = PROC_ON_SPELL_HIT;
    uint32_t victimProcFlags = PROC_ON_SPELL_HIT_VICTIM;

    // Get spell healing bonus
    if (isCreatureOrPlayer() && !staticDamage)
    {
        const auto casterUnit = static_cast<Unit*>(this);

        auto bonusHeal = casterUnit->getSpellHealingBonus(spellInfo, amt, isPeriodic, aur);
        if (isPeriodic && aur != nullptr)
            bonusHeal *= aur->getStackCount();

        heal += bonusHeal;
        if (heal < 0.0f)
            heal = 0.0f;
    }

    // Hackfixes from legacy method
    if (isCreatureOrPlayer())
    {
        const auto casterUnit = static_cast<Unit*>(this);
        switch (spellInfo->getId())
        {
            //SPELL_HASH_LESSER_HEALING_WAVE
            case 8004:
            case 8008:
            case 8010:
            case 10466:
            case 10467:
            case 10468:
            case 25420:
            case 27624:
            case 28849:
            case 28850:
            case 44256:
            case 46181:
            case 49275:
            case 49276:
            case 49309:
            case 66055:
            case 68115:
            case 68116:
            case 68117:
            case 75366:
            //SPELL_HASH_HEALING_WAVE
            case 331:
            case 332:
            case 547:
            case 913:
            case 939:
            case 959:
            case 8005:
            case 10395:
            case 10396:
            case 11986:
            case 12491:
            case 12492:
            case 15982:
            case 25357:
            case 25391:
            case 25396:
            case 26097:
            case 38330:
            case 43548:
            case 48700:
            case 49272:
            case 49273:
            case 51586:
            case 52868:
            case 55597:
            case 57785:
            case 58980:
            case 59083:
            case 60012:
            case 61569:
            case 67528:
            case 68318:
            case 69958:
            case 71133:
            case 75382:
            {
                //Tidal Waves
                casterUnit->RemoveAura(53390, casterUnit->getGuid());
            }
            //SPELL_HASH_CHAIN_HEAL
            case 1064:
            case 10622:
            case 10623:
            case 14900:
            case 15799:
            case 16367:
            case 25422:
            case 25423:
            case 33642:
            case 41114:
            case 42027:
            case 42477:
            case 43527:
            case 48894:
            case 54481:
            case 55458:
            case 55459:
            case 59473:
            case 69923:
            case 70425:
            case 71120:
            case 75370:
            {
                //Maelstrom Weapon
                casterUnit->removeAllAurasByIdForGuid(53817, casterUnit->getGuid());
            } break;
            case 54172: //Paladin - Divine Storm heal effect
            {
                int dmg = (int)CalculateDamage(casterUnit, victim, MELEE, nullptr, sSpellMgr.getSpellInfo(53385));    //1 hit
                int target = 0;

                for (const auto& itr : casterUnit->getInRangeObjectsSet())
                {
                    if (itr)
                    {
                        auto obj = itr;
                        if (itr->isCreatureOrPlayer() && static_cast<Unit*>(itr)->isAlive() && obj->isInRange(casterUnit, 8) && (casterUnit->GetPhase() & itr->GetPhase()))
                        {
                            // TODO: fix me!

                            //did_hit_result = Spell::DidHit(sSpellMgr.getSpellInfo(53385)->getEffect(0), static_cast<Unit*>(itr));
                            //if (did_hit_result == SPELL_DID_HIT_SUCCESS)
                                target++;
                        }
                    }
                }
                if (target > 4)
                    target = 4;

                heal = static_cast<float_t>((dmg * target) >> 2);   // 25%
            } break;
            default:
                break;
        }
    }

    // Check if heal can crit
    if (heal > 0.0f)
    {
        // If the heal is forced to be crit, do not alter it
        if (!isCritical && !(spellInfo->getAttributesExB() & ATTRIBUTESEXB_CANT_CRIT))
        {
            if (isPeriodic)
            {
                // For periodic effects use aura's own crit chance
                if (aur != nullptr && aur->getCritChance() > 0.0f)
                    isCritical = Rand(aur->getCritChance());
            }
            else
            {
                isCritical = IsCriticalHealForSpell(victim, spellInfo);
            }
        }

        // Get critical spell heal bonus
        if (isCritical)
        {
            heal = GetCriticalHealBonusForSpell(victim, spellInfo, heal);

            casterProcFlags |= PROC_ON_SPELL_CRIT_HIT;
            victimProcFlags |= PROC_ON_SPELL_CRIT_HIT_VICTIM;
        }
    }

    // Get target's heal taken mod
    heal += static_cast<float_t>(heal * victim->HealTakenPctMod[school]);
    if (isPeriodic)
    {
        if (aur != nullptr && aurEff != nullptr && aurEff->mAmplitude != 0)
            heal += static_cast<float_t>(victim->HealTakenMod[school] / (aur->getMaxDuration() / aurEff->mAmplitude));
    }
    else
    {
        heal += static_cast<float_t>(victim->HealTakenMod[school]);
    }

    if (heal < 0.0f)
        heal = 0.0f;

    uint32_t heal_int = 0;
    // For periodic auras convert the float amount to integer and save the heal fraction for next tick
    // but skip leech effects because the damage part only uses fractions
    if (isPeriodic && !isLeech && aur != nullptr && aurEff != nullptr)
    {
        heal += aurEff->mDamageFraction;
        if (aur->getTimeLeft() >= aurEff->mAmplitude)
        {
            heal_int = static_cast<uint32_t>(heal);
            aurEff->mDamageFraction = heal - heal_int;
        }
        else
        {
            // In case this is the last tick, just round the value
            heal_int = static_cast<uint32_t>(std::round(heal));
        }
    }
    else
    {
        // If this is a direct heal spell just round the value
        heal_int = static_cast<uint32_t>(std::round(heal));
    }

    uint32_t overHeal = 0;
    const auto curHealth = victim->getHealth();
    const auto maxHealth = victim->getMaxHealth();

    if ((curHealth + heal_int) >= maxHealth)
    {
        victim->setHealth(maxHealth);
        overHeal = curHealth + heal_int - maxHealth;
    }
    else
    {
        victim->modHealth(heal_int);
    }

    ///\ todo: implement (aura effect 301)
    const uint32_t absorbedHeal = 0;

    if (isPeriodic && aurEff != nullptr && isCreatureOrPlayer())
    {
        const auto wasPacketSent = victim->sendPeriodicAuraLog(GetNewGUID(), victim->GetNewGUID(), spellInfo, heal_int, overHeal, absorbedHeal, 0, aurEff->mAuraEffect, isCritical);

        // In case periodic log couldn't be sent, send normal spell log
        // Required for leech auras
        if (!wasPacketSent)
            victim->sendSpellHealLog(this, victim, spellId, heal_int, isCritical, overHeal, absorbedHeal);
    }
    else
    {
        victim->sendSpellHealLog(this, victim, spellId, heal_int, isCritical, overHeal, absorbedHeal);
    }

    // Handle procs
    if (isCreatureOrPlayer())
    {
        const auto casterUnit = static_cast<Unit*>(this);
        victim->HandleProc(victimProcFlags, casterUnit, spellInfo, !allowProc, heal_int);
        victim->m_procCounter = 0;
        casterUnit->HandleProc(casterProcFlags, victim, spellInfo, !allowProc, heal_int);
        casterUnit->m_procCounter = 0;
    }

    if (isPlayer())
    {
        const auto plr = static_cast<Player*>(this);

        // Check for pvp flag (healing a flagged target will flag caster)
        if (victim->isPlayer() && plr != victim)
        {
            const auto playerTarget = static_cast<Player*>(victim);
            if (playerTarget->isPvpFlagSet())
            {
                if (!plr->isPvpFlagSet())
                    plr->PvPToggle();
                else
                    plr->setPvpFlag();
            }
        }

        // Update battleground score
        plr->m_bgScore.HealingDone += heal_int;
        if (plr->m_bg != nullptr)
            plr->m_bg->UpdatePvPData();

        plr->last_heal_spell = spellInfo;
        plr->m_casted_amount[school] = heal_int;
    }

    victim->RemoveAurasByHeal();

    // Add threat
    if (isCreatureOrPlayer())
    {
        const auto casterUnit = static_cast<Unit*>(this);
        std::vector<Unit*> target_threat;
        int count = 0;
        for (const auto& itr : casterUnit->getInRangeObjectsSet())
        {
            if (!itr || !itr->isCreature())
                continue;

            auto tmp_creature = static_cast<Creature*>(itr);

            if (!tmp_creature->CombatStatus.IsInCombat() || (tmp_creature->GetAIInterface()->getThreatByPtr(casterUnit) == 0 && tmp_creature->GetAIInterface()->getThreatByPtr(victim) == 0))
                continue;

            if (!(casterUnit->GetPhase() & itr->GetPhase()))     //Can't see, can't be a threat
                continue;

            target_threat.push_back(tmp_creature);
            count++;
        }
        if (count == 0)
            return;

        auto heal_threat = heal_int / count;

        for (const auto& itr : target_threat)
        {
            itr->GetAIInterface()->HealReaction(casterUnit, victim, spellInfo, heal_threat);
        }

        if (victim->IsInWorld() && casterUnit->IsInWorld())
            casterUnit->CombatStatus.WeHealed(victim);
    }
}

void Object::_UpdateSpells(uint32_t time)
{
    if (m_currentSpell[CURRENT_AUTOREPEAT_SPELL] != nullptr && isPlayer())
    {
        static_cast<Player*>(this)->updateAutoRepeatSpell();
    }

    for (uint8_t i = 0; i < CURRENT_SPELL_MAX; ++i)
    {
        if (m_currentSpell[i] != nullptr)
        {
            // Remove finished spells from object's current spell array
            if (m_currentSpell[i]->getState() == SPELL_STATE_FINISHED)
            {
                m_currentSpell[i] = nullptr;
            }
            // Update spells with other state
            else
            {
                m_currentSpell[i]->Update(time);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// InRange sets
void Object::clearInRangeSets()
{
    mInRangeObjectsSet.clear();
    mInRangePlayersSet.clear();
    mInRangeOppositeFactionSet.clear();
    mInRangeSameFactionSet.clear();
}

void Object::addToInRangeObjects(Object* pObj)
{
    ARCEMU_ASSERT(pObj != nullptr);

    if (pObj == this)
        LOG_ERROR("We are in range of ourselves!");

    if (pObj->isPlayer())
        mInRangePlayersSet.push_back(pObj);

    mInRangeObjectsSet.push_back(pObj);
}

void Object::removeSelfFromInrangeSets()
{
    for (const auto& itr : mInRangeObjectsSet)
    {
        if (itr)
            itr->removeObjectFromInRangeObjectsSet(this);
    }
}

// Objects
std::vector<Object*> Object::getInRangeObjectsSet()
{
    return mInRangeObjectsSet;
}

bool Object::hasInRangeObjects()
{
    return mInRangeObjectsSet.size() > 0;
}

size_t Object::getInRangeObjectsCount()
{
    return mInRangeObjectsSet.size();
}

bool Object::isObjectInInRangeObjectsSet(Object* pObj)
{
    auto it = std::find(mInRangeObjectsSet.begin(), mInRangeObjectsSet.end(), pObj);
    return it != mInRangeObjectsSet.end();
}

void Object::removeObjectFromInRangeObjectsSet(Object* pObj)
{
    ARCEMU_ASSERT(pObj != nullptr);

    if (pObj->isPlayer())
        mInRangePlayersSet.erase(std::remove(mInRangePlayersSet.begin(), mInRangePlayersSet.end(), pObj), mInRangePlayersSet.end());

    mInRangeObjectsSet.erase(std::remove(mInRangeObjectsSet.begin(), mInRangeObjectsSet.end(), pObj), mInRangeObjectsSet.end());

    onRemoveInRangeObject(pObj);
}

// Players
std::vector<Object*> Object::getInRangePlayersSet()
{
    return mInRangePlayersSet;
}

size_t Object::getInRangePlayersCount()
{
    return mInRangePlayersSet.size();
}

// Opposite Faction
std::vector<Object*> Object::getInRangeOppositeFactionSet()
{
    return mInRangeOppositeFactionSet;
}

bool Object::isObjectInInRangeOppositeFactionSet(Object* pObj)
{
    auto it = std::find(mInRangeOppositeFactionSet.begin(), mInRangeOppositeFactionSet.end(), pObj);
    return it != mInRangeOppositeFactionSet.end();
}

void Object::updateInRangeOppositeFactionSet()
{
    mInRangeOppositeFactionSet.clear();

    for (const auto& itr : mInRangeObjectsSet)
    {
        if (itr)
        {
            if (itr->isCreatureOrPlayer() || itr->isGameObject())
            {
                if (isHostile(this, itr))
                {
                    if (!itr->isObjectInInRangeOppositeFactionSet(this))
                        itr->mInRangeOppositeFactionSet.push_back(this);
                    if (!isObjectInInRangeOppositeFactionSet(itr))
                        mInRangeOppositeFactionSet.push_back(itr);
                }
                else
                {
                    if (itr->isObjectInInRangeOppositeFactionSet(this))
                        itr->mInRangeOppositeFactionSet.erase(std::remove(itr->mInRangeOppositeFactionSet.begin(), itr->mInRangeOppositeFactionSet.end(), this), itr->mInRangeOppositeFactionSet.end());
                    if (isObjectInInRangeOppositeFactionSet(itr))
                        mInRangeOppositeFactionSet.erase(std::remove(mInRangeOppositeFactionSet.begin(), mInRangeOppositeFactionSet.end(), itr), mInRangeOppositeFactionSet.end());
                }
            }
        }
    }
}

void Object::addInRangeOppositeFaction(Object* obj)
{
    mInRangeOppositeFactionSet.push_back(obj);
}

void Object::removeObjectFromInRangeOppositeFactionSet(Object* obj)
{
    mInRangeOppositeFactionSet.erase(std::remove(mInRangeOppositeFactionSet.begin(), mInRangeOppositeFactionSet.end(), obj), mInRangeOppositeFactionSet.end());
}

// Same Faction
std::vector<Object*> Object::getInRangeSameFactionSet()
{
    return mInRangeSameFactionSet;
}

bool Object::isObjectInInRangeSameFactionSet(Object* pObj)
{
    auto it = std::find(mInRangeSameFactionSet.begin(), mInRangeSameFactionSet.end(), pObj);
    return it != mInRangeSameFactionSet.end();
}

void Object::updateInRangeSameFactionSet()
{
    mInRangeSameFactionSet.clear();

    for (const auto& itr : mInRangeObjectsSet)
    {
        if (itr)
        {
            if (itr->isCreatureOrPlayer() || itr->isGameObject())
            {
                if (isFriendly(this, itr))
                {
                    if (!itr->isObjectInInRangeSameFactionSet(this))
                        itr->mInRangeSameFactionSet.push_back(this);

                    if (!isObjectInInRangeOppositeFactionSet(itr))
                        mInRangeSameFactionSet.push_back(itr);
                }
                else
                {
                    if (itr->isObjectInInRangeSameFactionSet(this))
                        itr->mInRangeSameFactionSet.erase(std::remove(itr->mInRangeSameFactionSet.begin(), itr->mInRangeSameFactionSet.end(), this), itr->mInRangeSameFactionSet.end());

                    if (isObjectInInRangeSameFactionSet(itr))
                        mInRangeSameFactionSet.erase(std::remove(mInRangeSameFactionSet.begin(), mInRangeSameFactionSet.end(), itr), mInRangeSameFactionSet.end());
                }
            }
        }
    }
}

void Object::addInRangeSameFaction(Object* obj)
{
    mInRangeSameFactionSet.push_back(obj);
}

void Object::removeObjectFromInRangeSameFactionSet(Object* obj)
{
    mInRangeSameFactionSet.erase(std::remove(mInRangeSameFactionSet.begin(), mInRangeSameFactionSet.end(), obj), mInRangeSameFactionSet.end());
}

//////////////////////////////////////////////////////////////////////////////////////////
// Owner
Player* Object::getPlayerOwner() { return nullptr; }

//////////////////////////////////////////////////////////////////////////////////////////
// Misc
void Object::sendGameobjectDespawnAnim()
{
    SendMessageToSet(SmsgGameobjectDespawnAnim(this->getGuid()).serialise().get(), true);
}

// MIT End

Object::Object() : m_position(0, 0, 0, 0), m_spawnLocation(0, 0, 0, 0)
{
    m_mapId = MAPID_NOT_IN_WORLD;
    m_zoneId = 0;

    m_uint32Values = nullptr;
    m_objectUpdated = false;

    for (uint8_t i = 0; i < CURRENT_SPELL_MAX; ++i)
    {
        m_currentSpell[i] = nullptr;
    }
    m_valuesCount = 0;

    m_phase = 1;                //Set the default phase: 00000000 00000000 00000000 00000001

    m_mapMgr = nullptr;
    m_mapCell_x = m_mapCell_y = uint32(-1);

    m_factionTemplate = nullptr;
    m_factionEntry = nullptr;

    m_instanceId = INSTANCEID_NOT_IN_WORLD;
    Active = false;
    m_inQueue = false;
    m_loadedFromDB = false;

    m_objectType = TYPE_OBJECT;
    m_objectTypeId = TYPEID_OBJECT;
    m_updateFlag = UPDATEFLAG_NONE;

    mInRangeObjectsSet.clear();
    mInRangePlayersSet.clear();
    mInRangeOppositeFactionSet.clear();
    mInRangeSameFactionSet.clear();

    Active = false;
}

Object::~Object()
{
    if (!isItem())
        ARCEMU_ASSERT(!m_inQueue);

    ARCEMU_ASSERT(!IsInWorld());

    // for linux
    m_instanceId = INSTANCEID_NOT_IN_WORLD;

    for (uint8_t i = 0; i < CURRENT_SPELL_MAX; ++i)
    {
        if (m_currentSpell[i] != nullptr)
        {
            interruptSpellWithSpellType(CurrentSpellType(i));
        }
    }

    //avoid leaving traces in eventmanager. Have to work on the speed. Not all objects ever had events so list iteration can be skipped
    sEventMgr.RemoveEvents(this);
}

::DBC::Structures::AreaTableEntry const* Object::GetArea()
{
    if (!this->IsInWorld()) return nullptr;

    auto map_mgr = this->GetMapMgr();
    if (!map_mgr) return nullptr;

    auto area_flag = map_mgr->GetAreaFlag(this->GetPositionX(), this->GetPositionY(), this->GetPositionZ());
    auto at = MapManagement::AreaManagement::AreaStorage::GetAreaByFlag(area_flag);
    if (!at)
        at = MapManagement::AreaManagement::AreaStorage::GetAreaByMapId(this->GetMapId());

    return at;
}

void Object::_Create(uint32 mapid, float x, float y, float z, float ang)
{
    m_mapId = mapid;
    m_position.ChangeCoords({ x, y, z, ang });
    m_spawnLocation.ChangeCoords({ x, y, z, ang });
    m_lastMapUpdatePosition.ChangeCoords({ x, y, z, ang });
}

#if VERSION_STRING <= TBC
uint32 Object::buildCreateUpdateBlockForPlayer(ByteBuffer* data, Player* target)
{
    if (!target)
        return 0;

    uint8_t flags = 0, flags2 = 0, update_type = UPDATETYPE_CREATE_OBJECT;
    if (m_objectTypeId == TYPEID_CORPSE)
        if (static_cast<Corpse*>(this)->getDisplayId() == 0)
            return 0;

    switch (m_objectTypeId)
    {
    case TYPEID_ITEM:
    case TYPEID_CONTAINER:
        flags = 0x18;
        break;
    case TYPEID_UNIT:
    case TYPEID_PLAYER:
        flags = 0x70;
        break;
    case TYPEID_GAMEOBJECT:
    case TYPEID_DYNAMICOBJECT:
    case TYPEID_CORPSE:
        flags = 0x58;
        break;
    }

    if (target == this)
    {
        flags |= UPDATEFLAG_SELF;
        update_type = UPDATETYPE_CREATE_YOURSELF;
    }

    if (m_objectTypeId == TYPEID_GAMEOBJECT)
    {
        switch (((GameObject*)this)->getGoType())
        {
        case GAMEOBJECT_TYPE_MO_TRANSPORT:
            if (GetTypeFromGUID() != HIGHGUID_TYPE_TRANSPORTER)
                return 0;

            flags = 0x5a;
            break;
        case GAMEOBJECT_TYPE_TRANSPORT:
            flags = 0x5a;
            break;
        case GAMEOBJECT_TYPE_DUEL_ARBITER:
            update_type = UPDATETYPE_CREATE_YOURSELF;
            break;
        }
    }

    // we shouldn't be here, under any circumstances, unless we have a wowguid..
    ARCEMU_ASSERT(m_wowGuid.GetNewGuidLen() > 0);
    // build our actual update
    *data << update_type;
    *data << m_wowGuid;
    *data << m_objectTypeId;

    //\todo remove flags (0) from function call.
    buildMovementUpdate(data, flags, target);


    // we have dirty data, or are creating for ourself.
    UpdateMask update_mask;
    update_mask.SetCount(m_valuesCount);
    _SetCreateBits(&update_mask, target);

    // this will cache automatically if needed
    buildValuesUpdate(data, &update_mask, target);

    // update count: 1 ;)
    return 1;
}
#endif

#if VERSION_STRING > TBC
uint32 Object::buildCreateUpdateBlockForPlayer(ByteBuffer* data, Player* target)
{
    if (!target)
        return 0;

    uint8 updatetype = UPDATETYPE_CREATE_OBJECT;
    uint16 updateflags = m_updateFlag;

#if VERSION_STRING < Cata
    if (target == this)
    {
        updateflags |= UPDATEFLAG_SELF;
    }

    if (updateflags & UPDATEFLAG_HAS_POSITION)
    {
        if (IsType(TYPE_DYNAMICOBJECT) || IsType(TYPE_CORPSE) || IsType(TYPE_PLAYER))
            updatetype = UPDATETYPE_CREATE_YOURSELF;

        if (target->isPet() && updateflags & UPDATEFLAG_SELF)
            updatetype = UPDATETYPE_CREATE_YOURSELF;

        if (IsType(TYPE_GAMEOBJECT))
        {
            switch (((GameObject*)this)->getGoType())
            {
            case GAMEOBJECT_TYPE_TRAP:
            case GAMEOBJECT_TYPE_DUEL_ARBITER:
            case GAMEOBJECT_TYPE_FLAGSTAND:
            case GAMEOBJECT_TYPE_FLAGDROP:
                updatetype = UPDATETYPE_CREATE_YOURSELF;
                break;
            case GAMEOBJECT_TYPE_TRANSPORT:
                updateflags |= UPDATEFLAG_TRANSPORT;
                break;
            default:
                break;
            }
        }

        if (IsType(TYPE_UNIT))
        {
            if (((Unit*)this)->getTargetGuid() != 0)
                updateflags |= UPDATEFLAG_HAS_TARGET;
        }
    }

    if (isVehicle())
        updateflags |= UPDATEFLAG_VEHICLE;
#else
    switch (m_objectTypeId)
    {
    case TYPEID_CORPSE:
    case TYPEID_DYNAMICOBJECT:
        updateflags = UPDATEFLAG_HAS_POSITION; // UPDATEFLAG_HAS_STATIONARY_POSITION
        updatetype = 2;
        break;

    case TYPEID_GAMEOBJECT:
        updateflags = UPDATEFLAG_HAS_POSITION | UPDATEFLAG_ROTATION;
        //flags = 0x0040 | 0x0200; // UPDATEFLAG_HAS_STATIONARY_POSITION | UPDATEFLAG_ROTATION
        updatetype = 2;
        break;

    case TYPEID_UNIT:
    case TYPEID_PLAYER:
        updateflags = UPDATEFLAG_LIVING;
        updatetype = 2;
        break;
    }

    if (target == this)
    {
        // player creating self
        updateflags |= UPDATEFLAG_SELF;  // UPDATEFLAG_SELF
        updatetype = 2; // UPDATEFLAG_CREATE_OBJECT_SELF
    }

    if (isCreatureOrPlayer())
    {
        if (static_cast<Unit*>(this)->getTargetGuid())
            updateflags |= UPDATEFLAG_HAS_TARGET; // UPDATEFLAG_HAS_ATTACKING_TARGET
    }
#endif

    // we shouldn't be here, under any circumstances, unless we have a wowguid..
    ARCEMU_ASSERT(m_wowGuid.GetNewGuidLen() > 0);
    // build our actual update
    *data << uint8(updatetype);
    *data << m_wowGuid;
    *data << uint8(m_objectTypeId);

    buildMovementUpdate(data, updateflags, target);

    // we have dirty data, or are creating for ourself.
    UpdateMask updateMask;
    updateMask.SetCount(m_valuesCount);
    _SetCreateBits(&updateMask, target);

    // this will cache automatically if needed
    buildValuesUpdate(data, &updateMask, target);

    // update count: 1 ;)
    return 1;
}
#endif


//That is dirty fix it actually creates update of 1 field with
//the given value ignoring existing changes in fields and so on
//useful if we want update this field for certain players
//NOTE: it does not change fields. This is also very fast method
WorldPacket* Object::BuildFieldUpdatePacket(uint32 index, uint32 value)
{
    // uint64 guidfields = getGuid();
    // uint8 guidmask = 0;
    WorldPacket* packet = new WorldPacket(1500);
    packet->SetOpcode(SMSG_UPDATE_OBJECT);

    *packet << uint32(1);                   //number of update/create blocks
#if VERSION_STRING == TBC
    * packet << uint8(0);                    //unknown removed in 3.1
#endif

    *packet << uint8(UPDATETYPE_VALUES);    // update type == update
    *packet << GetNewGUID();

    uint32 mBlocks = index / 32 + 1;
    *packet << uint8(mBlocks);

    for (uint32 dword_n = mBlocks - 1; dword_n; dword_n--)
        *packet << uint32(0);

    *packet << (((uint32)(1)) << (index % 32));
    *packet << value;

    return packet;
}

void Object::BuildFieldUpdatePacket(Player* Target, uint32 Index, uint32 Value)
{
    ByteBuffer buf(500);
    buf << uint8(UPDATETYPE_VALUES);
    buf << GetNewGUID();

    uint32 mBlocks = Index / 32 + 1;
    buf << uint8(mBlocks);

    for (uint32 dword_n = mBlocks - 1; dword_n; dword_n--)
        buf << uint32(0);

    buf << (((uint32)(1)) << (Index % 32));
    buf << Value;

    Target->getUpdateMgr().pushUpdateData(&buf, 1);
}

void Object::BuildFieldUpdatePacket(ByteBuffer* buf, uint32 Index, uint32 Value)
{
    *buf << uint8(UPDATETYPE_VALUES);
    *buf << GetNewGUID();

    uint32 mBlocks = Index / 32 + 1;
    *buf << uint8(mBlocks);

    for (uint32 dword_n = mBlocks - 1; dword_n; dword_n--)
        *buf << uint32(0);

    *buf << (((uint32)(1)) << (Index % 32));
    *buf << Value;
}

uint32 Object::BuildValuesUpdateBlockForPlayer(ByteBuffer* data, Player* target)
{
    UpdateMask updateMask;
    updateMask.SetCount(m_valuesCount);
    _SetUpdateBits(&updateMask, target);
    for (uint32 x = 0; x < m_valuesCount; ++x)
    {
        if (updateMask.GetBit(x))
        {
            *data << uint8(UPDATETYPE_VALUES);              // update type == update
            ARCEMU_ASSERT(m_wowGuid.GetNewGuidLen() > 0);
            *data << m_wowGuid;

            buildValuesUpdate(data, &updateMask, target);
            return 1;
        }
    }

    return 0;
}

uint32 Object::BuildValuesUpdateBlockForPlayer(ByteBuffer* buf, UpdateMask* mask)
{
    // returns: update count
    // update type == update
    *buf << uint8(UPDATETYPE_VALUES);

    ARCEMU_ASSERT(m_wowGuid.GetNewGuidLen() > 0);
    *buf << m_wowGuid;

    buildValuesUpdate(buf, mask, nullptr);

    // 1 update.
    return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Build the Movement Data portion of the update packet Fills the data with this object's movement/speed info
///\todo rewrite this stuff, document unknown fields and flags
uint32 TimeStamp();

// MIT Start
#if VERSION_STRING < WotLK
void Object::buildMovementUpdate(ByteBuffer* data, uint8_t flags, Player* target)
{
    uint32_t flags2 = 0;
    // This is checked for nullptr later
    const auto spline_buffer = m_objectTypeId == TYPEID_UNIT ? target->getSplineMgr().popSplinePacket(getGuid()) : nullptr;

    *data << flags;

    Player* this_player = nullptr;
    if (getObjectTypeId() == TYPEID_PLAYER)
    {
        this_player = static_cast<Player*>(this);
        this_player->UpdateSpeed();
    }

    Creature* this_creature = nullptr;
    if (getObjectTypeId() == TYPEID_UNIT)
        this_creature = static_cast<Creature*>(this);

    if (flags & UPDATEFLAG_LIVING)
    {
        if (this_player && this_player->GetTransport())
        {
            flags2 |= MOVEFLAG_TRANSPORT;
        }
        else if (this_creature)
        {
            if (const auto transport = this_creature->GetTransport())
                if (transport->GetPosition().isSet())
                    flags2 |= MOVEFLAG_TRANSPORT;
        }

        if (this_creature)
        {
            switch (this_creature->getEntry())
            {
            case 6491:  // Spirit Healer
            case 13116: // Alliance Spirit Guide
            case 13117: // Horde Spirit Guide
                flags2 |= MOVEFLAG_WATER_WALK;
            default:
                break;
            }

            if (this_creature->GetAIInterface()->isFlying())
            {
                // Should this be MOVEFLAG_FLYING instead?
                flags2 |= MOVEFLAG_DISABLEGRAVITY;
            }

            const auto props = this_creature->GetCreatureProperties();
            if (props && props->extra_a9_flags)
            {
                if (!(flags2 & MOVEFLAG_TRANSPORT))
                {
                    flags2 |= props->extra_a9_flags;
                }
            }
        }

        *data << flags2;
        *data << uint8_t(0);
        *data << Util::getMSTime();
    }

    if (flags & UPDATEFLAG_HAS_POSITION)
    {
        *data << m_position << m_position.o;

        if (flags & UPDATEFLAG_LIVING && flags2 & MOVEFLAG_TRANSPORT)
        {
            if (this_player)
            {
                const auto transport = this_player->GetTransport();
                ARCEMU_ASSERT(transport != nullptr);

                *data << transport->GetNewGUID().getRawGuid();
                *data << transport->GetPosition() << transport->GetPosition().o;
                *data << transport->GetTransTime();
            }
            else if (this_creature && this_creature->GetTransport())
            {
                const auto transport = this_creature->GetTransport();
                *data << transport->GetNewGUID().getRawGuid();
                *data << transport->GetPosition() << transport->GetPosition().o;
                *data << transport->GetTransTime();
                *data << float(0.f); // unk
            }
        }
    }

    if (flags & UPDATEFLAG_LIVING)
    {
        *data << uint32_t(0); // unk

        if (flags2 & MOVEFLAG_FALLING)
        {
            *data << float(0);
            *data << float(1.0);
            *data << float(0);
            *data << float(0);
        }

        const Unit* this_mover = this_player ? static_cast<Unit*>(this_player) : this_creature;
        ARCEMU_ASSERT(this_mover != nullptr);

        *data << this_mover->getSpeedRate(TYPE_WALK, true);
        *data << this_mover->getSpeedRate(TYPE_RUN, true);
        *data << this_mover->getSpeedRate(TYPE_RUN_BACK, true);
        *data << this_mover->getSpeedRate(TYPE_SWIM, true);
        *data << this_mover->getSpeedRate(TYPE_SWIM_BACK, true);
        *data << this_mover->getSpeedRate(TYPE_FLY, true);
        *data << this_mover->getSpeedRate(TYPE_FLY_BACK, true);
        *data << this_mover->getSpeedRate(TYPE_TURN_RATE, true);
    }

    if (spline_buffer)
    {
        data->append(*spline_buffer);
        delete spline_buffer;
    }

    if (flags & UPDATEFLAG_LOWGUID)
    {
        *data << objectData()->guid_parts.low;
        if (flags & UPDATEFLAG_HIGHGUID)
            *data << objectData()->guid_parts.high;
    }
    else if (flags & UPDATEFLAG_HIGHGUID)
    {
        *data << objectData()->guid_parts.low;
    }

    if (flags & UPDATEFLAG_TRANSPORT)
    {
        *data << Util::getMSTime();
    }
}
#endif

#if VERSION_STRING == WotLK
void Object::buildMovementUpdate(ByteBuffer* data, uint16 flags, Player* target)
{
    uint32 flags2 = 0;

    ByteBuffer* splinebuf = (m_objectTypeId == TYPEID_UNIT) ? target->getSplineMgr().popSplinePacket(getGuid()) : 0;

    if (splinebuf != nullptr)
    {
        flags2 |= MOVEFLAG_SPLINE_ENABLED | MOVEFLAG_MOVE_FORWARD; //1=move forward
        if (isCreature())
        {
            if (static_cast<Unit*>(this)->GetAIInterface()->hasWalkMode(WALKMODE_WALK))
                flags2 |= MOVEFLAG_WALK;
        }
    }

    uint16 moveflags2 = 0;      // mostly seem to be used by vehicles to control what kind of movement is allowed

    if (isVehicle())
    {
        Unit* u = static_cast<Unit*>(this);
        if (u->getVehicleComponent() != nullptr)
            moveflags2 |= u->getVehicleComponent()->GetMoveFlags2();

        if (isCreature())
        {
            if (static_cast<Unit*>(this)->HasAuraWithName(SPELL_AURA_ENABLE_FLIGHT))
                flags2 |= (MOVEFLAG_DISABLEGRAVITY | MOVEFLAG_FLYING);
        }

    }

    *data << uint16(flags);

    Player* pThis = nullptr;
    MovementInfo* moveinfo = nullptr;
    if (isPlayer())
    {
        pThis = static_cast<Player*>(this);
        if (pThis->GetSession())
            moveinfo = pThis->GetSession()->GetMovementInfo();
    }
    Creature* uThis = nullptr;
    if (isCreature())
        uThis = static_cast<Creature*>(this);

    if (flags & UPDATEFLAG_LIVING)  //0x20
    {
        /*if (pThis && pThis->obj_movement_info.transporter_info.guid != 0)
            flags2 |= MOVEFLAG_TRANSPORT; //0x200
        else if (uThis != NULL && obj_movement_info.transporter_info.guid != 0 && uThis->obj_movement_info.transporter_info.guid != 0)
            flags2 |= MOVEFLAG_TRANSPORT; //0x200*/

            // Zyres: If a unit has this flag, add it to the update packet, otherwise not.
        if (pThis && pThis->hasUnitMovementFlag(MOVEFLAG_TRANSPORT))
            flags2 |= MOVEFLAG_TRANSPORT;
        else if (uThis && uThis->hasUnitMovementFlag(MOVEFLAG_TRANSPORT))
            flags2 |= MOVEFLAG_TRANSPORT;

        if ((pThis != nullptr) && pThis->isRooted())
            flags2 |= MOVEFLAG_ROOTED;
        else if ((uThis != nullptr) && uThis->isRooted())
            flags2 |= MOVEFLAG_ROOTED;

        if (uThis != nullptr)
        {
            // Don't know what this is, but I've only seen it applied to spirit healers. maybe some sort of invisibility flag? :/
            switch (getEntry())
            {
            case 6491:      // Spirit Healer
            case 13116:     // Alliance Spirit Guide
            case 13117:     // Horde Spirit Guide
            {
                flags2 |= MOVEFLAG_WATER_WALK;      //0x10000000
            }
            break;
            }

            if (uThis->GetAIInterface()->isFlying())
                flags2 |= MOVEFLAG_DISABLEGRAVITY;        //0x400 Zack : Teribus the Cursed had flag 400 instead of 800 and he is flying all the time
            if (uThis->GetAIInterface()->onGameobject)
                flags2 |= MOVEFLAG_ROOTED;
            if (uThis->GetCreatureProperties()->extra_a9_flags)
            {
                //do not send shit we can't honor
#define UNKNOWN_FLAGS2 (0x00002000 | 0x04000000 | 0x08000000)
                uint32 inherit = uThis->GetCreatureProperties()->extra_a9_flags & UNKNOWN_FLAGS2;
                flags2 |= inherit;
            }
        }

        *data << uint32(flags2);

        *data << uint16(moveflags2);

        *data << Util::getMSTime(); // this appears to be time in ms but can be any thing. Maybe packet serializer ?

        // this stuff:
        //   0x01 -> Enable Swimming?
        //   0x04 -> ??
        //   0x10 -> disables movement compensation and causes players to jump around all the place
        //   0x40 -> disables movement compensation and causes players to jump around all the place

        //Send position data, every living thing has these
        *data << float(m_position.x);
        *data << float(m_position.y);
        *data << float(m_position.z);
        *data << float(m_position.o);

        if (flags2 & MOVEFLAG_TRANSPORT) //0x0200
        {
            *data << WoWGuid(obj_movement_info.transport_guid);
            *data << float(GetTransPositionX());
            *data << float(GetTransPositionY());
            *data << float(GetTransPositionZ());
            *data << float(GetTransPositionO());
            *data << uint32(GetTransTime());
            *data << uint8(GetTransSeat());
        }

        if ((flags2 & (MOVEFLAG_SWIMMING | MOVEFLAG_FLYING)) || (moveflags2 & MOVEFLAG2_ALLOW_PITCHING))   // 0x2000000+0x0200000 flying/swimming, || sflags & SMOVE_FLAG_ENABLE_PITCH
        {
            if (pThis && moveinfo)
                *data << moveinfo->pitch_rate;
            else
                *data << float(0); //pitch
        }

        if (pThis && moveinfo)
            *data << moveinfo->fall_time;
        else
            *data << uint32(0); //last fall time

        if (flags2 & MOVEFLAG_FALLING)   // 0x00001000
        {
            if (moveinfo != nullptr)
            {
                *data << moveinfo->jump_info.velocity;
                *data << moveinfo->jump_info.sinAngle;
                *data << moveinfo->jump_info.cosAngle;
                *data << moveinfo->jump_info.xyspeed;
            }
            else
            {
                *data << float(0);
                *data << float(1.0);
                *data << float(0);
                *data << float(0);
            }
        }

        if (Unit* unit = static_cast<Unit*>(this))
        {
            *data << unit->getSpeedRate(TYPE_WALK, true);
            *data << unit->getSpeedRate(TYPE_RUN, true);
            *data << unit->getSpeedRate(TYPE_RUN_BACK, true);
            *data << unit->getSpeedRate(TYPE_SWIM, true);
            *data << unit->getSpeedRate(TYPE_SWIM_BACK, true);
            *data << unit->getSpeedRate(TYPE_FLY, true);
            *data << unit->getSpeedRate(TYPE_FLY_BACK, true);
            *data << unit->getSpeedRate(TYPE_TURN_RATE, true);
            *data << unit->getSpeedRate(TYPE_PITCH_RATE, true);
        }
        else                                //\todo Zyres: this is ridiculous... only units have these types, but this function is a mess so don't breake anything.
        {
            *data << float(2.5f);
            *data << float(7.0f);
            *data << float(4.5f);
            *data << float(4.722222f);
            *data << float(2.5f);
            *data << float(7.0f);
            *data << float(4.5f);
            *data << float(3.141594f);
            *data << float(3.14f);
        }

        if (flags2 & MOVEFLAG_SPLINE_ENABLED)   //VLack: On Mangos this is a nice spline movement code, but we never had such... Also, at this point we haven't got this flag, that's for sure, but fail just in case...
        {
            if (splinebuf != nullptr)
            {
                data->append(*splinebuf);
                //delete splinebuf;
            }
            else
                *data << float(0.0f);
        }
    }
    else        // No UPDATEFLAG_LIVING
    {
        if (flags & UPDATEFLAG_POSITION)        //0x0100
        {
            *data << uint8(0);                  //some say it is like parent guid ?
            *data << float(m_position.x);
            *data << float(m_position.y);
            *data << float(m_position.z);
            *data << float(m_position.x);
            *data << float(m_position.y);
            *data << float(m_position.z);
            *data << float(m_position.o);

            if (isCorpse())
                *data << float(m_position.o);   //VLack: repeat the orientation!
            else
                *data << float(0);
        }
        else if (flags & UPDATEFLAG_HAS_POSITION)  //0x40
        {
            if (flags & UPDATEFLAG_TRANSPORT &&  static_cast<GameObject*>(this)->getGoType() == GAMEOBJECT_TYPE_MO_TRANSPORT)
            {
                *data << float(0);
                *data << float(0);
                *data << float(0);
            }
            else
            {
                *data << float(m_position.x);
                *data << float(m_position.y);
                *data << float(m_position.z);
            }
            *data << float(m_position.o);
        }
    }


    if (flags & UPDATEFLAG_LOWGUID)     //0x08
        *data << getGuidLow();

    if (flags & UPDATEFLAG_HIGHGUID)    //0x10
        *data << getGuidHigh();

    if (flags & UPDATEFLAG_HAS_TARGET)  //0x04
    {
        if (isCreatureOrPlayer())
            FastGUIDPack(*data, static_cast<Unit*>(this)->getTargetGuid()); //some compressed GUID
        else
            *data << uint64(0);
    }

    if (flags & UPDATEFLAG_TRANSPORT)   //0x2
    {
        *data << Util::getMSTime();
    }

    if (flags & UPDATEFLAG_VEHICLE)
    {
        uint32 vehicleid = 0;

        if (isCreature())
            vehicleid = static_cast<Creature*>(this)->GetCreatureProperties()->vehicleid;
        else
            if (isPlayer())
                vehicleid = static_cast<Player*>(this)->mountvehicleid;

        *data << uint32(vehicleid);
        *data << float(GetOrientation());
    }

    if (flags & UPDATEFLAG_ROTATION)   //0x0200
    {
        if (isGameObject())
            *data << static_cast<GameObject*>(this)->GetRotation();
    }
}
#endif

#if VERSION_STRING >= Cata
void Object::buildMovementUpdate(ByteBuffer* data, uint16 updateFlags, Player* /*target*/)
{
    ObjectGuid Guid = getGuid();

    data->writeBit(false);
    data->writeBit(false);
    data->writeBit(updateFlags & UPDATEFLAG_ROTATION);
    data->writeBit(updateFlags & UPDATEFLAG_ANIM_KITS);
    data->writeBit(updateFlags & UPDATEFLAG_HAS_TARGET);
    data->writeBit(updateFlags & UPDATEFLAG_SELF);
    data->writeBit(updateFlags & UPDATEFLAG_VEHICLE);
    data->writeBit(updateFlags & UPDATEFLAG_LIVING);
    data->writeBits(0, 24);
    data->writeBit(false);
    data->writeBit(updateFlags & UPDATEFLAG_POSITION);
    data->writeBit(updateFlags & UPDATEFLAG_HAS_POSITION);
    data->writeBit(updateFlags & UPDATEFLAG_TRANSPORT_ARR);
    data->writeBit(false);
    data->writeBit(updateFlags & UPDATEFLAG_TRANSPORT);

    bool hasTransport = false;
    bool isSplineEnabled = false;
    bool hasPitch = false;
    bool hasFallData = false;
    bool hasFallDirection = false;
    bool hasElevation = false;
    bool hasOrientation = !IsType(TYPE_ITEM);
    bool hasTimeStamp = true;
    bool hasTransportTime2 = false;
    bool hasTransportTime3 = false;

    if (IsType(TYPE_UNIT))
    {
        Unit* unit = (Unit*)this;
        hasTransport = unit->movement_info.transport_guid != 0;
        isSplineEnabled = false; // unit->IsSplineEnabled();

        if (getObjectTypeId() == TYPEID_PLAYER)
        {
            hasPitch = unit->movement_info.getMovementStatusInfo().hasPitch;
            hasFallData = unit->movement_info.getMovementStatusInfo().hasFallData;
            hasFallDirection = unit->movement_info.getMovementStatusInfo().hasFallDirection;
            hasElevation = unit->movement_info.getMovementStatusInfo().hasSplineElevation;
            hasTransportTime2 = unit->movement_info.getMovementStatusInfo().hasTransportTime2;
            hasTransportTime3 = unit->movement_info.getMovementStatusInfo().hasTransportTime3;
        }
        else
        {
            hasPitch = unit->movement_info.hasMovementFlag(MovementFlags(MOVEFLAG_SWIMMING | MOVEFLAG_FLYING)) ||
                unit->movement_info.hasMovementFlag2(MOVEFLAG2_ALLOW_PITCHING);
            hasFallData = unit->movement_info.hasMovementFlag2(MOVEFLAG2_INTERPOLATED_TURN);
            hasFallDirection = unit->movement_info.hasMovementFlag(MOVEFLAG_FALLING);
            hasElevation = unit->movement_info.hasMovementFlag(MOVEFLAG_SPLINE_ELEVATION);
        }
    }

    if (updateFlags & UPDATEFLAG_LIVING)
    {
        Unit* unit = (Unit*)this;

        data->writeBit(!unit->movement_info.getMovementFlags());
        data->writeBit(!hasOrientation);

        data->writeBit(Guid[7]);
        data->writeBit(Guid[3]);
        data->writeBit(Guid[2]);

        if (unit->movement_info.getMovementFlags())
            data->writeBits(unit->movement_info.getMovementFlags(), 30);

        data->writeBit(false);
        data->writeBit(!hasPitch);
        data->writeBit(isSplineEnabled);
        data->writeBit(hasFallData);
        data->writeBit(!hasElevation);
        data->writeBit(Guid[5]);
        data->writeBit(hasTransport);
        data->writeBit(!hasTimeStamp);

        if (hasTransport)
        {
            ObjectGuid tGuid = unit->movement_info.transport_guid;

            data->writeBit(tGuid[1]);
            data->writeBit(hasTransportTime2);
            data->writeBit(tGuid[4]);
            data->writeBit(tGuid[0]);
            data->writeBit(tGuid[6]);
            data->writeBit(hasTransportTime3);
            data->writeBit(tGuid[7]);
            data->writeBit(tGuid[5]);
            data->writeBit(tGuid[3]);
            data->writeBit(tGuid[2]);
        }

        data->writeBit(Guid[4]);

        if (isSplineEnabled)
        {
            //Movement::PacketBuilder::WriteCreateBits(*unit->movespline, *data);
        }

        data->writeBit(Guid[6]);

        if (hasFallData)
            data->writeBit(hasFallDirection);

        data->writeBit(Guid[0]);
        data->writeBit(Guid[1]);

        data->writeBit(false);
        data->writeBit(!unit->movement_info.getMovementFlags2());

        if (unit->movement_info.getMovementFlags2())
            data->writeBits(unit->movement_info.getMovementFlags2(), 12);

    }

    if (updateFlags & UPDATEFLAG_POSITION)
    {
        ObjectGuid transGuid;

        data->writeBit(transGuid[5]);
        data->writeBit(hasTransportTime3);
        data->writeBit(transGuid[0]);
        data->writeBit(transGuid[3]);
        data->writeBit(transGuid[6]);
        data->writeBit(transGuid[1]);
        data->writeBit(transGuid[4]);
        data->writeBit(transGuid[2]);
        data->writeBit(hasTransportTime2);
        data->writeBit(transGuid[7]);
    }

    if (updateFlags & UPDATEFLAG_HAS_TARGET)
    {
        if (isCreatureOrPlayer())
        {
            ObjectGuid victimGuid = static_cast<Unit*>(this)->getTargetGuid();

            data->writeBit(victimGuid[2]);
            data->writeBit(victimGuid[7]);
            data->writeBit(victimGuid[0]);
            data->writeBit(victimGuid[4]);
            data->writeBit(victimGuid[5]);
            data->writeBit(victimGuid[6]);
            data->writeBit(victimGuid[1]);
            data->writeBit(victimGuid[3]);
        }
        else
            data->writeBits(0, 8);
    }

    if (updateFlags & UPDATEFLAG_ANIM_KITS)
    {
        data->writeBit(true);
        data->writeBit(true);
        data->writeBit(true);
    }

    data->flushBits();

    if (updateFlags & UPDATEFLAG_LIVING)
    {
        Unit* unit = (Unit*)this;

        data->WriteByteSeq(Guid[4]);

        *data << unit->getSpeedRate(TYPE_RUN_BACK, true);

        if (hasFallData)
        {
            if (hasFallDirection)
            {
                *data << float(unit->movement_info.getJumpInfo().cosAngle);
                *data << float(unit->movement_info.getJumpInfo().xyspeed);
                *data << float(unit->movement_info.getJumpInfo().sinAngle);
            }

            *data << uint32_t(unit->movement_info.getFallTime());
            *data << float(unit->movement_info.getJumpInfo().velocity);
        }

        *data << unit->getSpeedRate(TYPE_SWIM_BACK, true);

        if (hasElevation)
            *data << float(unit->movement_info.getSplineElevation());

        if (isSplineEnabled)
        {
            //Movement::PacketBuilder::WriteCreateBytes(*unit->movespline, *data);
        }

        *data << float(unit->GetPositionZ());
        data->WriteByteSeq(Guid[5]);

        if (hasTransport)
        {
            ObjectGuid tGuid = unit->movement_info.transport_guid;

            data->WriteByteSeq(tGuid[5]);
            data->WriteByteSeq(tGuid[7]);

            *data << uint32(unit->movement_info.transport_guid);
            *data << float(normalizeOrientation(unit->movement_info.transport_position.o));

            if (hasTransportTime2)
                *data << uint32_t(unit->movement_info.transport_time2);

            *data << float(unit->movement_info.transport_position.y);
            *data << float(unit->movement_info.transport_position.x);

            data->WriteByteSeq(tGuid[3]);

            *data << float(unit->movement_info.transport_position.z);

            data->WriteByteSeq(tGuid[0]);

            if (hasTransportTime3)
                *data << uint32_t(unit->movement_info.getFallTime());

            *data << int8_t(unit->movement_info.transport_seat);

            data->WriteByteSeq(tGuid[1]);
            data->WriteByteSeq(tGuid[6]);
            data->WriteByteSeq(tGuid[2]);
            data->WriteByteSeq(tGuid[4]);
        }

        *data << float(unit->GetPositionX());
        *data << float(unit->getSpeedRate(TYPE_PITCH_RATE, true));

        data->WriteByteSeq(Guid[3]);
        data->WriteByteSeq(Guid[0]);

        *data << float(unit->getSpeedRate(TYPE_SWIM, true));
        *data << float(unit->GetPositionY());

        data->WriteByteSeq(Guid[7]);
        data->WriteByteSeq(Guid[1]);
        data->WriteByteSeq(Guid[2]);

        *data << float(unit->getSpeedRate(TYPE_WALK, true));

        *data << uint32_t(Util::getMSTime());

        *data << float(unit->getSpeedRate(TYPE_FLY_BACK, true));

        data->WriteByteSeq(Guid[6]);

        *data << float(unit->getSpeedRate(TYPE_TURN_RATE, true));

        if (hasOrientation)
            *data << float(normalizeOrientation(unit->GetOrientation()));

        *data << unit->getSpeedRate(TYPE_RUN, true);

        if (hasPitch)
            *data << float(unit->movement_info.getPitch());

        *data << float(unit->getSpeedRate(TYPE_FLY, true));
    }

    if (updateFlags & UPDATEFLAG_VEHICLE)
    {
        uint32_t vehicleid = 0;

        if (isCreature())
        {
            vehicleid = static_cast<Creature*>(this)->GetCreatureProperties()->vehicleid;
        }
        else
        {
            if (isPlayer())
                vehicleid = static_cast<Player*>(this)->mountvehicleid;
        }

        *data << float(normalizeOrientation(((Object*)this)->GetOrientation()));
        *data << uint32_t(vehicleid);
    }

    if (updateFlags & UPDATEFLAG_POSITION)
    {
        ObjectGuid transGuid;

        data->WriteByteSeq(transGuid[0]);
        data->WriteByteSeq(transGuid[5]);

        if (hasTransportTime3)
            *data << uint32_t(0);

        data->WriteByteSeq(transGuid[3]);

        *data << float(0.0f);               // x offset

        data->WriteByteSeq(transGuid[4]);
        data->WriteByteSeq(transGuid[6]);
        data->WriteByteSeq(transGuid[1]);

        *data << uint32_t(0);               // transport time
        *data << float(0.0f);               // y offset

        data->WriteByteSeq(transGuid[2]);
        data->WriteByteSeq(transGuid[7]);

        *data << float(0.0f);               // z offset
        *data << int8_t(-1);                // transport seat
        *data << float(0.0f);               // o offset

        if (hasTransportTime2)
            *data << uint32_t(0);
    }

    if (updateFlags & UPDATEFLAG_ROTATION)
    {
        if (isGameObject())
            *data << int64_t(static_cast<GameObject*>(this)->GetRotation());
    }

    if (updateFlags & UPDATEFLAG_TRANSPORT_ARR)
    {
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << uint8_t(0);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
    }

    if (updateFlags & UPDATEFLAG_HAS_POSITION)
    {
        *data << float(normalizeOrientation(((Object*)this)->GetOrientation()));
        *data << float(((Object*)this)->GetPositionX());
        *data << float(((Object*)this)->GetPositionY());
        *data << float(((Object*)this)->GetPositionZ());
    }

    if (updateFlags & UPDATEFLAG_HAS_TARGET)
    {
        if (isCreatureOrPlayer())
        {
            ObjectGuid victimGuid = static_cast<Unit*>(this)->getTargetGuid();

            data->WriteByteSeq(victimGuid[4]);
            data->WriteByteSeq(victimGuid[0]);
            data->WriteByteSeq(victimGuid[3]);
            data->WriteByteSeq(victimGuid[5]);
            data->WriteByteSeq(victimGuid[7]);
            data->WriteByteSeq(victimGuid[6]);
            data->WriteByteSeq(victimGuid[2]);
            data->WriteByteSeq(victimGuid[1]);
        }
        else
        {
            for (uint8_t i = 0; i < 8; ++i)
            {
                *data << uint8_t(0);
            }
        }
    }

    if (updateFlags & UPDATEFLAG_TRANSPORT)
        *data << uint32_t(Util::getMSTime());
}
#endif

void Object::buildValuesUpdate(ByteBuffer* data, UpdateMask* updateMask, Player* target)
{
    auto activate_quest_object = false, reset = false;
    uint32_t old_flags = 0;

    // Create a new object
    if (updateMask->GetBit(getOffsetForStructuredField(WoWObject, guid)) && target)
    {
        if (isCreature())
        {
            auto this_creature = static_cast<Creature*>(this);
            if (this_creature->IsTagged() && this_creature->loot.any())
            {
                uint32_t current_flags;
                old_flags = current_flags = this_creature->getDynamicFlags();
                if (this_creature->GetTaggerGUID() == target->getGuid())
                {
                    old_flags = U_DYN_FLAG_TAGGED_BY_OTHER;
                    if (current_flags & U_DYN_FLAG_TAGGED_BY_OTHER)
                        current_flags &= ~old_flags;

                    if (!(current_flags & U_DYN_FLAG_LOOTABLE) && this_creature->HasLootForPlayer(target))
                        current_flags |= U_DYN_FLAG_LOOTABLE;
                }
                else
                {
                    old_flags = U_DYN_FLAG_LOOTABLE;

                    if (!(current_flags & U_DYN_FLAG_TAGGED_BY_OTHER))
                        current_flags |= U_DYN_FLAG_TAGGED_BY_OTHER;

                    if (current_flags & U_DYN_FLAG_LOOTABLE)
                        current_flags &= ~old_flags;
                }

                this_creature->setDynamicFlags(current_flags);
                reset = true;
            }

            if (isGameObject())
            {
                const auto this_go = static_cast<GameObject*>(this);
                if (this_go->isQuestGiver())
                {
                    auto this_qg = static_cast<GameObject_QuestGiver*>(this);
                    if (this_qg->HasQuests())
                    {
                        for (auto quest_relation : this_qg->getQuestList())
                        {
                            if (!quest_relation)
                                continue;

                            if (const auto quest_props = quest_relation->qst)
                            {
                                activate_quest_object = (quest_relation->type & QUESTGIVER_QUEST_START && !target->HasQuest(quest_props->id))
                                    || (quest_relation->type & QUESTGIVER_QUEST_END && !target->HasQuest(quest_props->id));
                            }
                        }
                    }
                    else
                    {
                        if (const auto go_props = this_go->GetGameObjectProperties())
                        {
                            if (go_props->goMap.size() > 0 || go_props->itemMap.size() > 0)
                            {
                                for (const auto quest_go : go_props->goMap)
                                {
                                    if (const auto quest_log = target->GetQuestLogForEntry(quest_go.first->id))
                                    {
                                        const auto quest = quest_log->getQuestProperties();
                                        if (quest->count_required_mob == 0)
                                            continue;

                                        for (auto i = 0; i < 4; ++i)
                                        {
                                            if (quest->required_mob_or_go[i] == static_cast<int32_t>(this_go->getEntry()))
                                            {
                                                if (quest_log->getMobCountByIndex(i) < quest->required_mob_or_go_count[i])
                                                {
                                                    activate_quest_object = true;
                                                    break;
                                                }
                                            }
                                        }
                                    }

                                    if (activate_quest_object)
                                        break;
                                }

                                if (!activate_quest_object)
                                {
                                    for (auto quest_props : go_props->itemMap)
                                    {
                                        for (const auto item_pair : quest_props.second)
                                        {
                                            if (const auto quest_log = target->GetQuestLogForEntry(quest_props.first->id))
                                            {
                                                if (target->getItemInterface()->GetItemCount(item_pair.first) < item_pair.second)
                                                {
                                                    activate_quest_object = true;
                                                    break;
                                                }
                                            }
                                        }

                                        if (activate_quest_object)
                                            break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // We already checked if GameObject, but do it again in case logic path changes in future
    if (activate_quest_object && isGameObject())
    {
        const auto this_go = static_cast<GameObject*>(this);
        old_flags = this_go->getDynamic();
        // Show sparkles
        this_go->setDynamic(1 | 8);
        reset = true;
    }

    ARCEMU_ASSERT(updateMask && updateMask->GetCount() == m_valuesCount);

    uint32_t block_count, values_count;
    if (m_valuesCount > 2 * 0x20)
    {
        block_count = updateMask->GetUpdateBlockCount();
        values_count = std::min<uint32_t>(block_count * 0x20, m_valuesCount);
    }
    else
    {
        block_count = updateMask->GetBlockCount();
        values_count = m_valuesCount;
    }

    *data << uint8_t(block_count);
    data->append(updateMask->GetMask(), block_count * 4);
    for (uint32_t idx = 0; idx < values_count; ++idx)
    {
        if (updateMask->GetBit(idx))
            *data << m_uint32Values[idx];
    }

    if (reset)
    {
        skipping_updates = true;
        if (isCreatureOrPlayer())
            static_cast<Unit*>(this)->setDynamicFlags(old_flags);
        else if (isGameObject())
            static_cast<GameObject*>(this)->setDynamic(old_flags);
        skipping_updates = false;
    }
}
// MIT End

// This is not called!
void Unit::BuildHeartBeatMsg(WorldPacket* data)
{
    data->Initialize(MSG_MOVE_HEARTBEAT, 32);
    *data << getGuid();
    BuildMovementPacket(data);
}

bool Object::SetPosition(const LocationVector & v, bool allowPorting /* = false */)
{
    bool updateMap = false, result = true;

    if (m_position.x != v.x || m_position.y != v.y)
        updateMap = true;

    m_position = const_cast<LocationVector &>(v);

#if VERSION_STRING < Cata
    if (!allowPorting && v.z < -500)
    {
        m_position.z = 500;
        LOG_ERROR("setPosition: fell through map; height ported");

        result = false;
    }
#endif

    if (IsInWorld() && updateMap)
    {
        m_mapMgr->ChangeObjectLocation(this);
    }

    return result;
}

bool Object::SetPosition(float newX, float newY, float newZ, float newOrientation, bool allowPorting)
{
    bool updateMap = false, result = true;

    ARCEMU_ASSERT(!std::isnan(newX) && !std::isnan(newY) && !std::isnan(newOrientation));

    //It's a good idea to push through EVERY transport position change, no matter how small they are. By: VLack aka. VLsoft
    if (isGameObject() && static_cast<GameObject*>(this)->GetGameObjectProperties()->type == GAMEOBJECT_TYPE_MO_TRANSPORT)
        updateMap = true;

    //if (m_position.x != newX || m_position.y != newY)
    //updateMap = true;
    if (m_lastMapUpdatePosition.Distance2DSq({ newX, newY }) > 4.0f) /* 2.0f */
        updateMap = true;

    m_position.ChangeCoords({ newX, newY, newZ, newOrientation });

#if VERSION_STRING < Cata
    if (!allowPorting && newZ < -500)
    {
        m_position.z = 500;
        LOG_ERROR("setPosition: fell through map; height ported");

        result = false;
    }
#endif

    if (IsInWorld() && updateMap)
    {
        m_lastMapUpdatePosition.ChangeCoords({ newX, newY, newZ, newOrientation });
        m_mapMgr->ChangeObjectLocation(this);

        if (isPlayer() && static_cast<Player*>(this)->GetGroup() && static_cast<Player*>(this)->m_last_group_position.Distance2DSq(m_position) > 25.0f)       // distance of 5.0
        {
            static_cast<Player*>(this)->AddGroupUpdateFlag(GROUP_UPDATE_FLAG_POSITION);
        }
    }

    if (isCreatureOrPlayer())
    {
        Unit* u = static_cast<Unit*>(this);
        if (u->getVehicleComponent() != nullptr)
            u->getVehicleComponent()->MovePassengers(newX, newY, newZ, newOrientation);
    }

    return result;
}

void Object::_SetUpdateBits(UpdateMask* updateMask, Player* /*target*/) const
{
    *updateMask = m_updateMask;
}

void Object::_SetCreateBits(UpdateMask* updateMask, Player* /*target*/) const
{
    for (uint32 i = 0; i < m_valuesCount; ++i)
        if (m_uint32Values[i] != 0)
            updateMask->SetBit(i);
}

void Object::AddToWorld()
{
    MapMgr* mapMgr = sInstanceMgr.GetInstance(this);
    if (mapMgr == nullptr)
    {
        LOG_ERROR("AddToWorld() failed for Object with GUID " I64FMT " MapId %u InstanceId %u", getGuid(), GetMapId(), GetInstanceID());
        return;
    }

    if (isPlayer())
    {
        Player* plr = static_cast<Player*>(this);
        if (mapMgr->pInstance != nullptr && !plr->isGMFlagSet())
        {
            // Player limit?
            if (mapMgr->GetMapInfo()->playerlimit && mapMgr->GetPlayerCount() >= mapMgr->GetMapInfo()->playerlimit)
                return;
            Group* group = plr->GetGroup();
            // Player in group?
            if (group == nullptr && mapMgr->pInstance->m_creatorGuid == 0)
                return;
            // If set: Owns player the instance?
            if (mapMgr->pInstance->m_creatorGuid != 0 && mapMgr->pInstance->m_creatorGuid != plr->getGuidLow())
                return;

            if (group != nullptr)
            {
                // Is instance empty or owns our group the instance?
                if (mapMgr->pInstance->m_creatorGroup != 0 && mapMgr->pInstance->m_creatorGroup != group->GetID())
                {
                    // Player not in group or another group is already playing this instance.
                    sChatHandler.SystemMessage(plr->GetSession(), "Another group is already inside this instance of the dungeon.");
                    if (plr->GetSession()->GetPermissionCount() > 0)
                        sChatHandler.BlueSystemMessage(plr->GetSession(), "Enable your GameMaster flag to ignore this rule.");
                    return;
                }
                else if (mapMgr->pInstance->m_creatorGroup == 0)
                    // Players group now "owns" the instance.
                    mapMgr->pInstance->m_creatorGroup = group->GetID();
            }
        }
    }

    m_mapMgr = mapMgr;
    m_inQueue = true;

    // correct incorrect instance id's
    m_instanceId = m_mapMgr->GetInstanceID();
    m_mapId = m_mapMgr->GetMapId();
    mapMgr->AddObject(this);
}

void Object::AddToWorld(MapMgr* pMapMgr)
{
    if (!pMapMgr || (pMapMgr->GetMapInfo()->playerlimit && this->isPlayer() && pMapMgr->GetPlayerCount() >= pMapMgr->GetMapInfo()->playerlimit))
        return; //instance add failed

    m_mapMgr = pMapMgr;
    m_inQueue = true;

    pMapMgr->AddObject(this);

    // correct incorrect instance id's
    m_instanceId = pMapMgr->GetInstanceID();
    m_mapId = m_mapMgr->GetMapId();
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Unlike addtoworld it pushes it directly ignoring add pool this can
/// only be called from the thread of mapmgr!
//////////////////////////////////////////////////////////////////////////////////////////
void Object::PushToWorld(MapMgr* mgr)
{
    ARCEMU_ASSERT(t_currentMapContext.get() == mgr);

    if (mgr == nullptr)
    {
        LOG_ERROR("Invalid push to world of Object " I64FMT, getGuid());
        return; //instance add failed
    }

    m_mapId = mgr->GetMapId();
    //there's no need to set the InstanceId before calling PushToWorld() because it's already set here.
    m_instanceId = mgr->GetInstanceID();

    if (isPlayer())
    {
        static_cast<Player*>(this)->m_cache->SetInt32Value(CACHE_MAPID, m_mapId);
        static_cast<Player*>(this)->m_cache->SetInt32Value(CACHE_INSTANCEID, m_instanceId);
    }

    m_mapMgr = mgr;
    OnPrePushToWorld();

    mgr->PushObject(this);

    // correct incorrect instance id's
    m_inQueue = false;

    event_Relocate();

    // call virtual function to handle stuff.. :P
    OnPushToWorld();
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Remove object from world
//////////////////////////////////////////////////////////////////////////////////////////
void Object::RemoveFromWorld(bool free_guid)
{
    ARCEMU_ASSERT(m_mapMgr != NULL);

    OnPreRemoveFromWorld();

    MapMgr* m = m_mapMgr;
    m_mapMgr = nullptr;

    m->RemoveObject(this, free_guid);

    OnRemoveFromWorld();

    std::set<Spell*>::iterator itr, itr2;
    Spell* sp;
    for (itr = m_pendingSpells.begin(); itr != m_pendingSpells.end();)
    {
        itr2 = itr++;
        sp = *itr2;
        //if the spell being deleted is the same being casted, Spell::cancel will take care of deleting the spell
        //if it's not the same removing us from world. Otherwise finish() will delete the spell once all SpellEffects are handled.
        bool foundSpell = false;
        for (uint8_t i = 0; i < CURRENT_SPELL_MAX; ++i)
        {
            if (sp == m_currentSpell[i])
            {
                interruptSpellWithSpellType(CurrentSpellType(i));
                foundSpell = true;
                break;
            }
        }
        if (!foundSpell)
            delete sp;
    }
    //shouldnt need to clear, spell destructor will erase
    //m_pendingSpells.clear();

    m_instanceId = INSTANCEID_NOT_IN_WORLD;
    m_mapId = MAPID_NOT_IN_WORLD;
    //m_inQueue is set to true when AddToWorld() is called. AddToWorld() queues the Object to be pushed, but if it's not pushed and RemoveFromWorld()
    //is called, m_inQueue will still be true even if the Object is no more inworld, nor queued.
    m_inQueue = false;

    // update our event holder
    event_Relocate();
}

float Object::CalcDistance(Object* Ob)
{
    ARCEMU_ASSERT(Ob != NULL);
    return CalcDistance(this->GetPositionX(), this->GetPositionY(), this->GetPositionZ(), Ob->GetPositionX(), Ob->GetPositionY(), Ob->GetPositionZ());
}

float Object::CalcDistance(float ObX, float ObY, float ObZ)
{
    return CalcDistance(this->GetPositionX(), this->GetPositionY(), this->GetPositionZ(), ObX, ObY, ObZ);
}

float Object::CalcDistance(Object* Oa, Object* Ob)
{
    ARCEMU_ASSERT(Oa != NULL);
    ARCEMU_ASSERT(Ob != NULL);
    return CalcDistance(Oa->GetPositionX(), Oa->GetPositionY(), Oa->GetPositionZ(), Ob->GetPositionX(), Ob->GetPositionY(), Ob->GetPositionZ());
}

float Object::CalcDistance(Object* Oa, float ObX, float ObY, float ObZ)
{
    ARCEMU_ASSERT(Oa != NULL);
    return CalcDistance(Oa->GetPositionX(), Oa->GetPositionY(), Oa->GetPositionZ(), ObX, ObY, ObZ);
}

float Object::CalcDistance(float OaX, float OaY, float OaZ, float ObX, float ObY, float ObZ)
{
    float xdest = OaX - ObX;
    float ydest = OaY - ObY;
    float zdest = OaZ - ObZ;
    return sqrtf(zdest * zdest + ydest * ydest + xdest * xdest);
}

bool Object::IsWithinDistInMap(Object* obj, const float dist2compare) const
{
    ARCEMU_ASSERT(obj != NULL);
    float xdest = this->GetPositionX() - obj->GetPositionX();
    float ydest = this->GetPositionY() - obj->GetPositionY();
    float zdest = this->GetPositionZ() - obj->GetPositionZ();
    return sqrtf(zdest * zdest + ydest * ydest + xdest * xdest) <= dist2compare;
}

bool Object::IsWithinLOSInMap(Object* obj)
{
    ARCEMU_ASSERT(obj != NULL);
    if (!IsInMap(obj)) return false;
    LocationVector location;
    location = obj->GetPosition();
    return IsWithinLOS(location);
}

bool Object::IsWithinLOS(LocationVector location)
{
    LocationVector location2;
    location2 = GetPosition();

    if (worldConfig.terrainCollision.isCollisionEnabled)
    {
        VMAP::IVMapManager* mgr = VMAP::VMapFactory::createOrGetVMapManager();
        return mgr->isInLineOfSight(GetMapId(), location2.x, location2.y, location2.z + 2.0f, location.x, location.y, location.z + 2.0f);
    }
    else
    {
        return true;
    }
}

float Object::calcAngle(float Position1X, float Position1Y, float Position2X, float Position2Y)
{
    float dx = Position2X - Position1X;
    float dy = Position2Y - Position1Y;
    double angle = 0.0f;

    // Calculate angle
    if (dx == 0.0)
    {
        if (dy == 0.0)
            angle = 0.0;
        else if (dy > 0.0)
            angle = M_PI * 0.5 /* / 2 */;
        else
            angle = M_PI * 3.0 * 0.5/* / 2 */;
    }
    else if (dy == 0.0)
    {
        if (dx > 0.0)
            angle = 0.0;
        else
            angle = M_PI;
    }
    else
    {
        if (dx < 0.0)
            angle = atanf(dy / dx) + M_PI;
        else if (dy < 0.0)
            angle = atanf(dy / dx) + (2 * M_PI);
        else
            angle = atanf(dy / dx);
    }

    // Convert to degrees
    angle = angle * float(180 / M_PI);

    // Return
    return float(angle);
}

float Object::calcRadAngle(float Position1X, float Position1Y, float Position2X, float Position2Y)
{
    double dx = double(Position2X - Position1X);
    double dy = double(Position2Y - Position1Y);
    double angle = 0.0;

    // Calculate angle
    if (dx == 0.0)
    {
        if (dy == 0.0)
            angle = 0.0;
        else if (dy > 0.0)
            angle = M_PI * 0.5/*/ 2.0*/;
        else
            angle = M_PI * 3.0 * 0.5/*/ 2.0*/;
    }
    else if (dy == 0.0)
    {
        if (dx > 0.0)
            angle = 0.0;
        else
            angle = M_PI;
    }
    else
    {
        if (dx < 0.0)
            angle = atan(dy / dx) + M_PI;
        else if (dy < 0.0)
            angle = atan(dy / dx) + (2 * M_PI);
        else
            angle = atan(dy / dx);
    }

    // Return
    return float(angle);
}

float Object::getEasyAngle(float angle)
{
    while (angle < 0)
    {
        angle = angle + 360;
    }
    while (angle >= 360)
    {
        angle = angle - 360;
    }
    return angle;
}

bool Object::inArc(float Position1X, float Position1Y, float FOV, float Orientation, float Position2X, float Position2Y)
{
    float angle = calcAngle(Position1X, Position1Y, Position2X, Position2Y);
    float lborder = getEasyAngle((Orientation - (FOV * 0.5f/*/2*/)));
    float rborder = getEasyAngle((Orientation + (FOV * 0.5f/*/2*/)));
    //LOG_DEBUG("Orientation: %f Angle: %f LeftBorder: %f RightBorder %f",Orientation,angle,lborder,rborder);
    if (((angle >= lborder) && (angle <= rborder)) || ((lborder > rborder) && ((angle < rborder) || (angle > lborder))))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool Object::isInFront(Object* target)
{
    // check if we facing something (is the object within a 180 degree slice of our positive y axis)

    double x = target->GetPositionX() - m_position.x;
    double y = target->GetPositionY() - m_position.y;

    double angle = atan2(y, x);
    angle = (angle >= 0.0) ? angle : 2.0 * M_PI + angle;
    angle -= m_position.o;

    while (angle > M_PI)
        angle -= 2.0 * M_PI;

    while (angle < -M_PI)
        angle += 2.0 * M_PI;

    // replace M_PI in the two lines below to reduce or increase angle

    double left = -1.0 * (M_PI / 2.0);
    double right = (M_PI / 2.0);

    return((angle >= left) && (angle <= right));
}

bool Object::isInBack(Object* target)
{
    // check if we are behind something (is the object within a 180 degree slice of our negative y axis)

    double x = m_position.x - target->GetPositionX();
    double y = m_position.y - target->GetPositionY();

    double angle = atan2(y, x);
    angle = (angle >= 0.0) ? angle : 2.0 * M_PI + angle;

    // if we are a creature and have a UNIT_FIELD_TARGET then we are always facing them
    if (isCreature() && static_cast<Creature*>(this)->getTargetGuid() != 0)
    {
        Unit* pTarget = static_cast<Creature*>(this)->GetAIInterface()->getNextTarget();
        if (pTarget != nullptr)
            angle -= double(Object::calcRadAngle(target->m_position.x, target->m_position.y, pTarget->m_position.x, pTarget->m_position.y));
        else
            angle -= target->GetOrientation();
    }
    else
        angle -= target->GetOrientation();

    while (angle > M_PI)
        angle -= 2.0 * M_PI;

    while (angle < -M_PI)
        angle += 2.0 * M_PI;

    // replace M_H_PI in the two lines below to reduce or increase angle

    double left = -1.0 * (M_H_PI / 2.0);
    double right = (M_H_PI / 2.0);

    return((angle <= left) && (angle >= right));
}

bool Object::isInArc(Object* target, float angle) // angle in degrees
{
    return inArc(GetPositionX(), GetPositionY(), angle, GetOrientation(), target->GetPositionX(), target->GetPositionY());
}

bool Object::HasInArc(float degrees, Object* target)
{
    return isInArc(target, degrees);
}

bool Object::isInRange(Object* target, float range)
{
    if (!this->IsInWorld() || !target) return false;
    float dist = CalcDistance(target);
    return(dist <= range);
}

void Object::setServersideFaction()
{
    DBC::Structures::FactionTemplateEntry const* faction_template = nullptr;

    if (isCreatureOrPlayer())
    {
        faction_template = sFactionTemplateStore.LookupEntry(static_cast<Unit*>(this)->getFactionTemplate());
        if (faction_template == nullptr)
            LOG_ERROR("Unit does not have a valid faction. Faction: %u set to Entry: %u", static_cast<Unit*>(this)->getFactionTemplate(), getEntry());
    }
    else if (isGameObject())
    {
        uint32 go_faction_id = static_cast<GameObject*>(this)->getFactionTemplate();
        faction_template = sFactionTemplateStore.LookupEntry(go_faction_id);
        if (go_faction_id != 0)         // faction = 0 means it has no faction.
        {
            if (faction_template == nullptr)
            {
                LOG_ERROR("GameObject does not have a valid faction. Faction: %u set to Entry: %u", static_cast<GameObject*>(this)->getFactionTemplate(), getEntry());
            }
        }
    }

    //this solution looks a bit off, but our db is not perfect and this prevents some crashes.
    m_factionTemplate = faction_template;
    if (m_factionTemplate == nullptr)
    {
        m_factionTemplate = sFactionTemplateStore.LookupEntry(0);
        m_factionEntry = sFactionStore.LookupEntry(0);
    }
    else
    {
        m_factionEntry = sFactionStore.LookupEntry(m_factionTemplate->Faction);
    }
}

uint32 Object::getServersideFaction()
{
    return m_factionTemplate->Faction;
}

void Object::DealDamage(Unit* /*pVictim*/, uint32 /*damage*/, uint32 /*targetEvent*/, uint32 /*unitEvent*/, uint32 /*spellId*/, bool /*no_remove_auras*/)
{}

//////////////////////////////////////////////////////////////////////////////////////////
/// SpellLog packets just to keep the code cleaner and better to read
//////////////////////////////////////////////////////////////////////////////////////////
void Object::SendSpellLog(Object* Caster, Object* Target, uint32 Ability, uint8 SpellLogType)
{
    if (Caster == nullptr || Target == nullptr || Ability == 0)
        return;

    Caster->SendMessageToSet(SmsgSpellLogMiss(Ability, Caster->getGuid(), Target->getGuid(), SpellLogType).serialise().get(), true);
}

#if VERSION_STRING <= TBC
void Object::SendAttackerStateUpdate(Object* Caster, Object* Target, dealdamage* Dmg, uint32 Damage, uint32 Abs, uint32 BlockedDamage, uint32 HitStatus, uint32 VState)
{
    if (!Caster || !Target || !Dmg)
        return;

    WorldPacket data(SMSG_ATTACKERSTATEUPDATE, 70); //guessed size

    data << uint32(HitStatus);
    data << Caster->GetNewGUID();
    data << Target->GetNewGUID();

    data << uint32(Damage);                 // Realdamage
    data << uint8(1);                       // Damage type counter / swing type

    data << uint32(g_spellSchoolConversionTable[Dmg->school_type]);         // Damage school
    data << float(Dmg->full_damage);        // Damage float
    data << uint32(Dmg->full_damage);       // Damage amount
    data << uint32(Abs);                    // Damage absorbed
    data << uint32(Dmg->resisted_damage);

    data << uint8(VState);
    data << uint32(0x03E8);          // can be 0,1000 or -1
    data << uint32(0);
    data << uint32(BlockedDamage);  // Damage amount blocked


    SendMessageToSet(&data, Caster->isPlayer());
}
#else
void Object::SendAttackerStateUpdate(Object* Caster, Object* Target, dealdamage* Dmg, uint32 Damage, uint32 Abs, uint32 BlockedDamage, uint32 HitStatus, uint32 VState)
{
    if (!Caster || !Target || !Dmg)
        return;

    WorldPacket data(SMSG_ATTACKERSTATEUPDATE, 108);

    uint32 Overkill = 0;

    if (Target->isCreatureOrPlayer() && Damage > static_cast<Unit*>(Target)->getHealth())
        Overkill = Damage - static_cast<Unit*>(Target)->getHealth();

    data << uint32(HitStatus);
    data << Caster->GetNewGUID();
    data << Target->GetNewGUID();

    data << uint32(Damage);                 // Realdamage
    data << uint32(Overkill);               // Overkill
    data << uint8(1);                       // Damage type counter / swing type

    data << uint32(g_spellSchoolConversionTable[Dmg->school_type]);         // Damage school
    data << float(Dmg->full_damage);        // Damage float
    data << uint32(Dmg->full_damage);       // Damage amount

    if (HitStatus & HITSTATUS_ABSORBED)
        data << uint32(Abs);                // Damage absorbed

    if (HitStatus & HITSTATUS_RESIST)
        data << uint32(Dmg->resisted_damage);   // Damage resisted

    data << uint8(VState);
    data << uint32(0);          // can be 0,1000 or -1
    data << uint32(0);

    if (HitStatus & HITSTATUS_BLOCK)
        data << uint32(BlockedDamage);  // Damage amount blocked


    if (HitStatus & HITSTATUS_RAGE_GAIN)
        data << uint32(0);              // unknown

    if (HitStatus & HITSTATUS_UNK_00)
    {
        data << uint32(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);
        data << float(0);

        data << float(0);       // Found in loop
        data << float(0);       // Found in loop
        data << uint32(0);
    }

    SendMessageToSet(&data, Caster->isPlayer());
}
#endif

int32 Object::event_GetInstanceID()
{
    // \return -1 for non-inworld.. so we get our shit moved to the right thread
    // \return default value is -1, if it's something else then we are/will be soon InWorld.
    return m_instanceId;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Object has an active state
//////////////////////////////////////////////////////////////////////////////////////////
bool Object::CanActivate()
{
    switch (m_objectTypeId)
    {
    case TYPEID_UNIT:
    {
        if (!isPet())
            return true;
    }
    break;

    case TYPEID_GAMEOBJECT:
    {
        if (static_cast<GameObject*>(this)->getGoType() != GAMEOBJECT_TYPE_TRAP)
            return true;
    }
    break;
    }

    return false;
}

void Object::Activate(MapMgr* mgr)
{
    switch (m_objectTypeId)
    {
    case TYPEID_UNIT:
        mgr->activeCreatures.insert(static_cast<Creature*>(this));
        break;

    case TYPEID_GAMEOBJECT:
        mgr->activeGameObjects.insert(static_cast<GameObject*>(this));
        break;
    }
    // Objects are active so set to true.
    Active = true;
}

void Object::Deactivate(MapMgr* mgr)
{
    if (mgr == nullptr)
        return;

    switch (m_objectTypeId)
    {
    case TYPEID_UNIT:
        // check iterator
        if (mgr->creature_iterator != mgr->activeCreatures.end() && (*mgr->creature_iterator)->getGuid() == getGuid())
            ++mgr->creature_iterator;
        mgr->activeCreatures.erase(static_cast<Creature*>(this));
        break;

    case TYPEID_GAMEOBJECT:
        mgr->activeGameObjects.erase(static_cast<GameObject*>(this));
        break;
    }
    Active = false;
}

void Object::SetZoneId(uint32 newZone)
{
    m_zoneId = newZone;

    if (isPlayer())
    {
        static_cast<Player*>(this)->m_cache->SetUInt32Value(CACHE_PLAYER_ZONEID, newZone);
        if (static_cast<Player*>(this)->GetGroup() != nullptr)
            static_cast<Player*>(this)->AddGroupUpdateFlag(GROUP_UPDATE_FLAG_ZONE);
    }
}

void Object::PlaySoundToSet(uint32 sound_entry)
{
    SendMessageToSet(SmsgPlaySound(sound_entry).serialise().get(), true);
}

bool Object::IsInBg()
{
    MySQLStructure::MapInfo const* pMapinfo = sMySQLStore.getWorldMapInfo(GetMapId());

    if (pMapinfo != nullptr)
    {
        return (pMapinfo->type == INSTANCE_BATTLEGROUND);
    }

    return false;
}

uint32 Object::GetTeam()
{

    switch (m_factionEntry->ID)
    {
        // Human
    case 1:
        // Dwarf
    case 3:
        // Nightelf
    case 4:
        // Gnome
    case 8:
        // Draenei
    case 927:
        return TEAM_ALLIANCE;

        // Orc
    case 2:
        // Undead
    case 5:
        // Tauren
    case 6:
        // Troll
    case 9:
        // Bloodelf
    case 914:
        return TEAM_HORDE;
    }

    return static_cast<uint32>(-1);
}

Transporter* Object::GetTransport() const
{
#if VERSION_STRING < Cata
    return sObjectMgr.GetTransporter(WoWGuid::getGuidLowPartFromUInt64(obj_movement_info.transport_guid));
#else
    return nullptr;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Manipulates the phase value, see "enum PHASECOMMANDS" in
/// Object.h for a longer explanation!
//////////////////////////////////////////////////////////////////////////////////////////
void Object::Phase(uint8 command, uint32 newphase)
{
    switch (command)
    {
    case PHASE_SET:
        m_phase = newphase;
        break;
    case PHASE_ADD:
        m_phase |= newphase;
        break;
    case PHASE_DEL:
        m_phase &= ~newphase;
        break;
    case PHASE_RESET:
        m_phase = 1;
        break;
    default:
        ARCEMU_ASSERT(false);
    }

    return;
}

void Object::OutPacketToSet(uint16 Opcode, uint16 Len, const void* Data, bool /*self*/)
{
    if (!IsInWorld())
        return;

    // We are on Object level, which means we can't send it to ourselves so we only send to Players inrange
    for (const auto& itr : mInRangePlayersSet)
    {
        if (itr)
            itr->OutPacket(Opcode, Len, Data);
    }
}

void Object::SendMessageToSet(WorldPacket* data, bool /*bToSelf*/, bool /*myteam_only*/)
{
    if (!IsInWorld())
        return;

    uint32 myphase = GetPhase();
    for (const auto& itr : mInRangePlayersSet)
    {
        if (itr && (itr->GetPhase() & myphase) != 0)
            itr->SendPacket(data);
    }
}

void Object::SendCreatureChatMessageInRange(Creature* creature, uint32_t textId)
{
    uint32 myphase = GetPhase();
    for (const auto& itr : mInRangePlayersSet)
    {
        Object* object = itr;
        if (object && (object->GetPhase() & myphase) != 0)
        {
            if (object->isPlayer())
            {
                Player* player = static_cast<Player*>(object);
                uint32_t sessionLanguage = player->GetSession()->language;

                std::string message;
                MySQLStructure::NpcScriptText const* npcScriptText = sMySQLStore.getNpcScriptText(textId);
                if (npcScriptText == nullptr)
                {
                    LOG_ERROR("Invalid textId: %u. This text is send by a script but not in table npc_script_text!", textId);
                    return;
                }

                MySQLStructure::LocalesNpcScriptText const* lnpct = (sessionLanguage > 0) ? sMySQLStore.getLocalizedNpcScriptText(textId, sessionLanguage) : nullptr;
                if (lnpct != nullptr)
                {
                    message = lnpct->text;
                }
                else
                {
                    message = npcScriptText->text;
                }

                std::string creatureName;

                MySQLStructure::LocalesCreature const* lcn = (sessionLanguage > 0) ? sMySQLStore.getLocalizedCreature(creature->getEntry(), sessionLanguage) : nullptr;
                if (lcn != nullptr)
                {
                    creatureName = lcn->name;
                }
                else
                {
                    creatureName = creature->GetCreatureProperties()->Name;
                }

                size_t creatureNameLength = creatureName.length() + 1;
                size_t messageLength = message.length() + 1;

                if (npcScriptText->emote != 0)
                {
                    creature->EventAddEmote((EmoteType)npcScriptText->emote, npcScriptText->duration);
                }

                if (npcScriptText->sound != 0)
                {
                    creature->PlaySoundToSet(npcScriptText->sound);
                }

                WorldPacket data(SMSG_MESSAGECHAT, 35 + creatureNameLength + messageLength);
                data << uint8_t(npcScriptText->type);
                data << uint32_t(npcScriptText->language);
                data << uint64_t(getGuid());
                data << uint32_t(0);
                data << uint32_t(creatureNameLength);
                data << creatureName;
                data << uint64_t(0);
                data << uint32_t(messageLength);
                data << message;
                data << uint8_t(0);
                player->SendPacket(&data);
            }
        }
    }
}

void Object::SendMonsterSayMessageInRange(Creature* creature, MySQLStructure::NpcMonsterSay* npcMonsterSay, int randChoice, uint32_t event)
{
    uint32 myphase = GetPhase();
    for (const auto& itr : mInRangePlayersSet)
    {
        Object* object = itr;
        if (object && (object->GetPhase() & myphase) != 0)
        {
            if (object->isPlayer())
            {
                Player* player = static_cast<Player*>(object);
                uint32_t sessionLanguage = player->GetSession()->language;

                //////////////////////////////////////////////////////////////////////////////////////////////
                // get text (normal or localized)
                const char* text = npcMonsterSay->texts[randChoice];
                MySQLStructure::LocalesNPCMonstersay const* lmsay = (sessionLanguage > 0) ? sMySQLStore.getLocalizedMonsterSay(getEntry(), sessionLanguage, event) : nullptr;
                if (lmsay != nullptr)
                {
                    switch (randChoice)
                    {
                    case 0:
                        if (lmsay->text0 != nullptr)
                            text = lmsay->text0;
                        break;
                    case 1:
                        if (lmsay->text1 != nullptr)
                            text = lmsay->text1;
                        break;
                    case 2:
                        if (lmsay->text2 != nullptr)
                            text = lmsay->text2;
                        break;
                    case 3:
                        if (lmsay->text3 != nullptr)
                            text = lmsay->text3;
                        break;
                    case 4:
                        if (lmsay->text4 != nullptr)
                            text = lmsay->text4;
                        break;
                    default:
                        text = npcMonsterSay->texts[randChoice];
                    }
                }
                else
                {
                    text = npcMonsterSay->texts[randChoice];
                }

                // replace text with content
                std::string newText = text;
#if VERSION_STRING < Cata
#if VERSION_STRING > Classic
                static const char* races[NUM_RACES] = { "None", "Human", "Orc", "Dwarf", "Night Elf", "Undead", "Tauren", "Gnome", "Troll", "None", "Blood Elf", "Draenei" };
#else
                static const char* races[NUM_RACES] = { "None", "Human", "Orc", "Dwarf", "Night Elf", "Undead", "Tauren", "Gnome", "Troll" };

#endif
#else
                static const char* races[NUM_RACES] = { "None", "Human", "Orc", "Dwarf", "Night Elf", "Undead", "Tauren", "Gnome", "Troll", "Goblin", "Blood Elf", "Draenei", "None", "None", "None", "None", "None", "None", "None", "None", "None", "None", "Worgen" };
#endif
                static const char* classes[MAX_PLAYER_CLASSES] = { "None", "Warrior", "Paladin", "Hunter", "Rogue", "Priest", "Death Knight", "Shaman", "Mage", "Warlock", "None", "Druid" };
                char* test = strstr((char*)text, "$R");
                if (test == nullptr)
                    test = strstr((char*)text, "$r");
                if (test != nullptr)
                {
                    uint64 targetGUID = creature->getTargetGuid();
                    Unit* CurrentTarget = GetMapMgr()->GetUnit(targetGUID);
                    if (CurrentTarget)
                    {
                        ptrdiff_t testOfs = test - text;
                        newText.replace(testOfs, 2, races[CurrentTarget->getRace()]);
                    }
                }
                test = strstr((char*)text, "$N");
                if (test == nullptr)
                    test = strstr((char*)text, "$n");
                if (test != nullptr)
                {
                    uint64 targetGUID = creature->getTargetGuid();
                    Unit* CurrentTarget = GetMapMgr()->GetUnit(targetGUID);
                    if (CurrentTarget && CurrentTarget->isPlayer())
                    {
                        ptrdiff_t testOfs = test - text;
                        newText.replace(testOfs, 2, static_cast<Player*>(CurrentTarget)->getName().c_str());
                    }
                }
                test = strstr((char*)text, "$C");
                if (test == nullptr)
                    test = strstr((char*)text, "$c");
                if (test != nullptr)
                {
                    uint64 targetGUID = creature->getTargetGuid();
                    Unit* CurrentTarget = GetMapMgr()->GetUnit(targetGUID);
                    if (CurrentTarget)
                    {
                        ptrdiff_t testOfs = test - text;
                        newText.replace(testOfs, 2, classes[CurrentTarget->getClass()]);
                    }
                }
                test = strstr((char*)text, "$G");
                if (test == nullptr)
                    test = strstr((char*)text, "$g");
                if (test != nullptr)
                {
                    uint64 targetGUID = creature->getTargetGuid();
                    Unit* CurrentTarget = GetMapMgr()->GetUnit(targetGUID);
                    if (CurrentTarget)
                    {
                        char* g0 = test + 2;
                        char* g1 = strchr(g0, ':');
                        if (g1)
                        {
                            char* gEnd = strchr(g1, ';');
                            if (gEnd)
                            {
                                *g1 = 0x00;
                                ++g1;
                                *gEnd = 0x00;
                                ++gEnd;
                                *test = 0x00;
                                newText = text;
                                newText += (CurrentTarget->getGender() == 0) ? g0 : g1;
                                newText += gEnd;
                            }
                        }
                    }
                }

                ////////////////////////////////////////////////////////////////////////////////////////////

                std::string creatureName;

                MySQLStructure::LocalesCreature const* lcn = (sessionLanguage > 0) ? sMySQLStore.getLocalizedCreature(creature->getEntry(), sessionLanguage) : nullptr;
                if (lcn != nullptr)
                {
                    creatureName = lcn->name;
                }
                else
                {
                    creatureName = creature->GetCreatureProperties()->Name;
                }

                size_t creatureNameLength = creatureName.length() + 1;
                size_t messageLength = newText.length() + 1;

                WorldPacket data(SMSG_MESSAGECHAT, 35 + creatureNameLength + messageLength);
                data << uint8_t(npcMonsterSay->type);
                data << uint32_t(npcMonsterSay->language);
                data << uint64_t(getGuid());
                data << uint32_t(0);
                data << uint32_t(creatureNameLength);
                data << creatureName;
                data << uint64_t(0);
                data << uint32_t(messageLength);
                data << newText;
                data << uint8_t(0);
                player->SendPacket(&data);
            }
        }
    }
}

Object* Object::GetMapMgrObject(const uint64 & guid)
{
    if (!IsInWorld())
        return nullptr;

    return GetMapMgr()->_GetObject(guid);
}

Pet* Object::GetMapMgrPet(const uint64 & guid)
{
    if (!IsInWorld())
        return nullptr;

    WoWGuid wowGuid;
    wowGuid.Init(guid);

    return GetMapMgr()->GetPet(wowGuid.getGuidLowPart());
}

Unit* Object::GetMapMgrUnit(const uint64 & guid)
{
    if (!IsInWorld())
        return nullptr;

    return GetMapMgr()->GetUnit(guid);
}

Player* Object::GetMapMgrPlayer(const uint64 & guid)
{
    if (!IsInWorld())
        return nullptr;

    WoWGuid wowGuid;
    wowGuid.Init(guid);

    return GetMapMgr()->GetPlayer(wowGuid.getGuidLowPart());
}

Creature* Object::GetMapMgrCreature(const uint64 & guid)
{
    if (!IsInWorld())
        return nullptr;

    WoWGuid wowGuid;
    wowGuid.Init(guid);

    return GetMapMgr()->GetCreature(wowGuid.getGuidLowPart());
}

GameObject* Object::GetMapMgrGameObject(const uint64 & guid)
{
    if (!IsInWorld())
        return nullptr;

    WoWGuid wowGuid;
    wowGuid.Init(guid);

    return GetMapMgr()->GetGameObject(wowGuid.getGuidLowPart());
}

DynamicObject* Object::GetMapMgrDynamicObject(const uint64 & guid)
{
    if (!IsInWorld())
        return nullptr;

    WoWGuid wowGuid;
    wowGuid.Init(guid);

    return GetMapMgr()->GetDynamicObject(wowGuid.getGuidLowPart());
}

MapCell* Object::GetMapCell() const
{
    ARCEMU_ASSERT(m_mapMgr != NULL);
    return m_mapMgr->GetCell(m_mapCell_x, m_mapCell_y);
}

void Object::SetMapCell(MapCell* cell)
{
    if (cell == nullptr)
    {
        //mapcell coordinates are uint16, so using uint32(-1) will always make GetMapCell() return NULL.
        m_mapCell_x = m_mapCell_y = uint32(-1);
    }
    else
    {
        m_mapCell_x = cell->GetPositionX();
        m_mapCell_y = cell->GetPositionY();
    }
}

void Object::SendAIReaction(uint32 reaction)
{
    SendMessageToSet(SmsgAiReaction(getGuid(), reaction).serialise().get(), false);
}

void Object::SendDestroyObject()
{
    SendMessageToSet(AscEmu::Packets::SmsgDestroyObject(getGuid()).serialise().get(), false);
}

bool Object::GetPoint(float angle, float rad, float & outx, float & outy, float & outz, bool sloppypath)
{
    if (!IsInWorld())
        return false;
    outx = GetPositionX() + rad * cos(angle);
    outy = GetPositionY() + rad * sin(angle);
    outz = GetMapMgr()->GetLandHeight(outx, outy, GetPositionZ() + 2);
    float waterz;
    uint32 watertype;
    GetMapMgr()->GetLiquidInfo(outx, outy, GetPositionZ() + 2, waterz, watertype);
    outz = std::max(waterz, outz);

    MMAP::MMapManager* mmap = MMAP::MMapFactory::createOrGetMMapManager();
    dtNavMesh* nav = const_cast<dtNavMesh*>(mmap->GetNavMesh(GetMapId()));
    dtNavMeshQuery* nav_query = const_cast<dtNavMeshQuery*>(mmap->GetNavMeshQuery(GetMapId(), GetInstanceID()));
    //NavMeshData* nav = CollideInterface.GetNavMesh(GetMapId());

    if (nav != nullptr)
    {
        //if we can path there, go for it
        if (!isCreatureOrPlayer() || !sloppypath || !static_cast<Unit*>(this)->GetAIInterface()->CanCreatePath(outx, outy, outz))
        {
            //raycast nav mesh to see if this place is valid
            float start[3] = { GetPositionY(), GetPositionZ() + 0.5f, GetPositionX() };
            float end[3] = { outy, outz + 0.5f, outx };
            float extents[3] = { 3, 5, 3 };
            dtQueryFilter filter;
            filter.setIncludeFlags(NAV_GROUND | NAV_WATER | NAV_SLIME | NAV_MAGMA);

            dtPolyRef startref;
            nav_query->findNearestPoly(start, extents, &filter, &startref, nullptr);

            float point;
            float hitNormal[3];
            float result[3];
            int numvisited;
            dtPolyRef visited[MAX_PATH_LENGTH];

            dtStatus rayresult = nav_query->raycast(startref, start, end, &filter, &point, hitNormal, visited, &numvisited, MAX_PATH_LENGTH);

            if (point <= 1.0f)
            {
                if (numvisited == 0 || rayresult == DT_FAILURE)
                {
                    result[0] = start[0];
                    result[1] = start[1];
                    result[2] = start[2];
                }
                else
                {
                    result[0] = start[0] + ((end[0] - start[0]) * point);
                    result[1] = start[1] + ((end[1] - start[1]) * point);
                    result[2] = start[2] + ((end[2] - start[2]) * point);
                    nav_query->getPolyHeight(visited[numvisited - 1], result, &result[1]);
                }

                //copy end back to function floats
                outy = result[0];
                outz = result[1];
                outx = result[2];
            }
        }
    }
    else //test against vmap if mmap isn't available
    {
        float testx, testy, testz;

        VMAP::IVMapManager* mgr = VMAP::VMapFactory::createOrGetVMapManager();
        bool isHittingObject = mgr->getObjectHitPos(GetMapId(), GetPositionX(), GetPositionY(), GetPositionZ() + 2, outx, outy, outz + 2, testx, testy, testz, -0.5f);

        if (isHittingObject)
        {
            //hit something
            outx = testx;
            outy = testy;
            outz = testz;
        }

        outz = GetMapMgr()->GetLandHeight(outx, outy, outz + 2);
    }

    return true;
}

#if VERSION_STRING >= Cata


void MovementInfo::readMovementInfo(ByteBuffer& data, uint16_t opcode)
{
    bool hasTransportData = false,
        hasMovementFlags = false,
        hasMovementFlags2 = false;

    MovementStatusElements* sequence = GetMovementStatusElementsSequence(sOpcodeTables.getInternalIdForHex(opcode));
    if (!sequence)
    {
        LogError("Unsupported MovementInfo::Read for 0x%X (%s)!", opcode);
        return;
    }

    for (uint32_t i = 0; i < MSE_COUNT; ++i)
    {
        MovementStatusElements element = sequence[i];
        if (element == MSEEnd)
            break;

        if (element >= MSEGuidBit0 && element <= MSEGuidBit7)
        {
            guid[element - MSEGuidBit0] = data.readBit();
            continue;
        }

        if (element >= MSEGuid2Bit0 && element <= MSEGuid2Bit7)
        {
            guid2[element - MSEGuid2Bit0] = data.readBit();
            continue;
        }

        if (element >= MSETransportGuidBit0 && element <= MSETransportGuidBit7)
        {
            if (hasTransportData)
                transport_guid[element - MSETransportGuidBit0] = data.readBit();
            continue;
        }

        if (element >= MSEGuidByte0 && element <= MSEGuidByte7)
        {
            if (guid[element - MSEGuidByte0])
                guid[element - MSEGuidByte0] ^= data.readUInt8();
            continue;
        }

        if (element >= MSEGuid2Byte0 && element <= MSEGuid2Byte7)
        {
            if (guid2[element - MSEGuid2Byte0])
                guid2[element - MSEGuid2Byte0] ^= data.readUInt8();
            continue;
        }

        if (element >= MSETransportGuidByte0 && element <= MSETransportGuidByte7)
        {
            if (hasTransportData && transport_guid[element - MSETransportGuidByte0])
                transport_guid[element - MSETransportGuidByte0] ^= data.readUInt8();
            continue;
        }

        switch (element)
        {
            case MSEFlags:
                if (hasMovementFlags)
                    flags = data.readBits(30);
                break;
            case MSEFlags2:
                if (hasMovementFlags2)
                    flags2 = static_cast<uint16_t>(data.readBits(12));
                break;
            case MSEHasUnknownBit:
                data.readBit();
                break;
            case MSETimestamp:
                if (status_info.hasTimeStamp)
                    data >> update_time;
                break;
            case MSEHasTimestamp:
                status_info.hasTimeStamp = !data.readBit();
                break;
            case MSEHasOrientation:
                status_info.hasOrientation = !data.readBit();
                break;
            case MSEHasMovementFlags:
                hasMovementFlags = !data.readBit();
                break;
            case MSEHasMovementFlags2:
                hasMovementFlags2 = !data.readBit();
                break;
            case MSEHasPitch:
                status_info.hasPitch = !data.readBit();
                break;
            case MSEHasFallData:
                status_info.hasFallData = data.readBit();
                break;
            case MSEHasFallDirection:
                if (status_info.hasFallData)
                    status_info.hasFallDirection = data.readBit();
                break;
            case MSEHasTransportData:
                hasTransportData = data.readBit();
                break;
            case MSEHasTransportTime2:
                if (hasTransportData)
                    status_info.hasTransportTime2 = data.readBit();
                break;
            case MSEHasTransportTime3:
                if (hasTransportData)
                    status_info.hasTransportTime3 = data.readBit();
                break;
            case MSEHasSpline:
                status_info.hasSpline = data.readBit();
                break;
            case MSEHasSplineElevation:
                status_info.hasSplineElevation = !data.readBit();
                break;
            case MSEPositionX:
                data >> position.x;
                break;
            case MSEPositionY:
                data >> position.y;
                break;
            case MSEPositionZ:
                data >> position.z;
                break;
            case MSEPositionO:
                if (status_info.hasOrientation)
                    data >> position.o;
                break;
            case MSEPitch:
                if (status_info.hasPitch)
                    data >> pitch_rate;
                break;
            case MSEFallTime:
                if (status_info.hasFallData)
                    data >> fall_time;
                break;
            case MSESplineElevation:
                if (status_info.hasSplineElevation)
                    data >> spline_elevation;
                break;
            case MSEFallHorizontalSpeed:
                if (status_info.hasFallData && status_info.hasFallDirection)
                    data >> jump_info.xyspeed;
                break;
            case MSEFallVerticalSpeed:
                if (status_info.hasFallData)
                    data >> jump_info.velocity;
                break;
            case MSEFallCosAngle:
                if (status_info.hasFallData && status_info.hasFallDirection)
                    data >> jump_info.cosAngle;
                break;
            case MSEFallSinAngle:
                if (status_info.hasFallData && status_info.hasFallDirection)
                    data >> jump_info.sinAngle;
                break;
            case MSETransportSeat:
                if (hasTransportData)
                    data >> transport_seat;
                break;
            case MSETransportPositionO:
                if (hasTransportData)
                    data >> transport_position.o;
                break;
            case MSETransportPositionX:
                if (hasTransportData)
                    data >> transport_position.x;
                break;
            case MSETransportPositionY:
                if (hasTransportData)
                    data >> transport_position.y;
                break;
            case MSETransportPositionZ:
                if (hasTransportData)
                    data >> transport_position.z;
                break;
            case MSETransportTime:
                if (hasTransportData)
                    data >> transport_time;
                break;
            case MSETransportTime2:
                if (hasTransportData && status_info.hasTransportTime2)
                    data >> transport_time2;
                break;
            case MSETransportTime3:
                if (hasTransportData && status_info.hasTransportTime3)
                    data >> fall_time;
                break;
            case MSEMovementCounter:
                data.read_skip<uint32_t>();
                break;
            case MSEByteParam:
                data >> byte_parameter;
                break;
            default:
                ARCEMU_ASSERT(false && "Wrong movement status element");
                break;
        }
    }
}

void MovementInfo::writeMovementInfo(ByteBuffer& data, uint16_t opcode, float custom_speed) const
{
    bool hasTransportData = !transport_guid.IsEmpty();

    MovementStatusElements* sequence = GetMovementStatusElementsSequence(opcode);
    if (!sequence)
    {
        LogError("Unsupported MovementInfo::Write for 0x%X!", opcode);
        return;
    }

    for (uint32_t i = 0; i < MSE_COUNT; ++i)
    {
        MovementStatusElements element = sequence[i];

        if (element == MSEEnd)
            break;

        if (element >= MSEGuidBit0 && element <= MSEGuidBit7)
        {
            data.writeBit(guid[element - MSEGuidBit0]);
            continue;
        }

        if (element >= MSETransportGuidBit0 && element <= MSETransportGuidBit7)
        {
            if (hasTransportData)
                data.writeBit(transport_guid[element - MSETransportGuidBit0]);
            continue;
        }

        if (element >= MSEGuidByte0 && element <= MSEGuidByte7)
        {
            if (guid[element - MSEGuidByte0])
                data << uint8_t((guid[element - MSEGuidByte0] ^ 1));
            continue;
        }

        if (element >= MSETransportGuidByte0 && element <= MSETransportGuidByte7)
        {
            if (hasTransportData && transport_guid[element - MSETransportGuidByte0])
                data << uint8_t((transport_guid[element - MSETransportGuidByte0] ^ 1));
            continue;
        }

        switch (element)
        {
            case MSEHasMovementFlags:
                data.writeBit(!flags);
                break;
            case MSEHasMovementFlags2:
                data.writeBit(!flags2);
                break;
            case MSEFlags:
                if (flags)
                    data.writeBits(flags, 30);
                break;
            case MSEFlags2:
                if (flags2)
                    data.writeBits(flags2, 12);
                break;
            case MSETimestamp:
                if (status_info.hasTimeStamp)
                    data << Util::getMSTime();
                break;
            case MSEHasPitch:
                data.writeBit(!status_info.hasPitch);
                break;
            case MSEHasTimestamp:
                data.writeBit(!status_info.hasTimeStamp);
                break;
            case MSEHasUnknownBit:
                data.writeBit(false);
                break;
            case MSEHasFallData:
                data.writeBit(status_info.hasFallData);
                break;
            case MSEHasFallDirection:
                if (status_info.hasFallData)
                    data.writeBit(status_info.hasFallDirection);
                break;
            case MSEHasTransportData:
                data.writeBit(hasTransportData);
                break;
            case MSEHasTransportTime2:
                if (hasTransportData)
                    data.writeBit(status_info.hasTransportTime2);
                break;
            case MSEHasTransportTime3:
                if (hasTransportData)
                    data.writeBit(status_info.hasTransportTime3);
                break;
            case MSEHasSpline:
                data.writeBit(status_info.hasSpline);
                break;
            case MSEHasSplineElevation:
                data.writeBit(!status_info.hasSplineElevation);
                break;
            case MSEPositionX:
                data << float(position.x);
                break;
            case MSEPositionY:
                data << float(position.y);
                break;
            case MSEPositionZ:
                data << float(position.z);
                break;
            case MSEPositionO:
                if (status_info.hasOrientation)
                    data << float(normalizeOrientation(position.o));
                break;
            case MSEPitch:
                if (status_info.hasPitch)
                    data << float(pitch_rate);
                break;
            case MSEHasOrientation:
                data.writeBit(!status_info.hasOrientation);
                break;
            case MSEFallTime:
                if (status_info.hasFallData)
                    data << uint32_t(fall_time);
                break;
            case MSESplineElevation:
                if (status_info.hasSplineElevation)
                    data << float(spline_elevation);
                break;
            case MSEFallHorizontalSpeed:
                if (status_info.hasFallData && status_info.hasFallDirection)
                    data << float(jump_info.xyspeed);
                break;
            case MSEFallVerticalSpeed:
                if (status_info.hasFallData)
                    data << float(jump_info.velocity);
                break;
            case MSEFallCosAngle:
                if (status_info.hasFallData && status_info.hasFallDirection)
                    data << float(jump_info.cosAngle);
                break;
            case MSEFallSinAngle:
                if (status_info.hasFallData && status_info.hasFallDirection)
                    data << float(jump_info.sinAngle);
                break;
            case MSETransportSeat:
                if (hasTransportData)
                    data << int8_t(transport_seat);
                break;
            case MSETransportPositionO:
                if (hasTransportData)
                    data << float(normalizeOrientation(transport_position.o));
                break;
            case MSETransportPositionX:
                if (hasTransportData)
                    data << float(transport_position.x);
                break;
            case MSETransportPositionY:
                if (hasTransportData)
                    data << float(transport_position.y);
                break;
            case MSETransportPositionZ:
                if (hasTransportData)
                    data << float(transport_position.z);
                break;
            case MSETransportTime:
                if (hasTransportData)
                    data << uint32_t(transport_time);
                break;
            case MSETransportTime2:
                if (hasTransportData && status_info.hasTransportTime2)
                    data << uint32_t(transport_time2);
                break;
            case MSETransportTime3:
                if (hasTransportData && status_info.hasTransportTime3)
                    data << uint32_t(fall_time);
                break;
            case MSEMovementCounter:
                data << uint32_t(0);
                break;
            case MSECustomSpeed:
                data << float(custom_speed);
                break;
            default:
                ARCEMU_ASSERT(false && "Wrong movement status element");
                break;
        }
    }
}

#endif
