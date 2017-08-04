/*
Copyright (c) 2014-2017 AscEmu Team <http://www.ascemu.org/>
This file is released under the MIT license. See README-MIT for more information.
*/

#include "Unit.h"
#include "Server/Packets/Opcode.h"
#include "Server/WorldSession.h"
#include "Players/Player.h"
#include "Spell/SpellAuras.h"
#include "Spell/Definitions/DiminishingGroup.h"
#include "Spell/Customization/SpellCustomizations.hpp"
#include "Objects/ObjectMgr.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Movement

void Unit::setMoveWaterWalk()
{
    AddUnitMovementFlag(MOVEFLAG_WATER_WALK);

    if (IsPlayer())
    {
        WorldPacket data(SMSG_MOVE_WATER_WALK, 12);
#if VERSION_STRING != Cata
        data << GetNewGUID();
        data << uint32(0);
#else
        movement_info.writeMovementInfo(data, SMSG_MOVE_WATER_WALK);
#endif
        SendMessageToSet(&data, true);
    }

    if (IsCreature())
    {
        WorldPacket data(SMSG_SPLINE_MOVE_WATER_WALK, 9);
#if VERSION_STRING != Cata
        data << GetNewGUID();
#else
        movement_info.writeMovementInfo(data, SMSG_SPLINE_MOVE_WATER_WALK);
#endif
        SendMessageToSet(&data, false);
    }
}

void Unit::setMoveLandWalk()
{
    RemoveUnitMovementFlag(MOVEFLAG_WATER_WALK);

    if (IsPlayer())
    {
        WorldPacket data(SMSG_MOVE_LAND_WALK, 12);
#if VERSION_STRING != Cata
        data << GetNewGUID();
        data << uint32(0);
#else
        movement_info.writeMovementInfo(data, SMSG_MOVE_LAND_WALK);
#endif
        SendMessageToSet(&data, true);
    }

    if (IsCreature())
    {
        WorldPacket data(SMSG_SPLINE_MOVE_LAND_WALK, 9);
#if VERSION_STRING != Cata
        data << GetNewGUID();
#else
        movement_info.writeMovementInfo(data, SMSG_SPLINE_MOVE_LAND_WALK);
#endif
        SendMessageToSet(&data, false);
    }
}

void Unit::setMoveFeatherFall()
{
    AddUnitMovementFlag(MOVEFLAG_FEATHER_FALL);

    if (IsPlayer())
    {
        WorldPacket data(SMSG_MOVE_FEATHER_FALL, 12);
#if VERSION_STRING != Cata
        data << GetNewGUID();
        data << uint32(0);
#else
        movement_info.writeMovementInfo(data, SMSG_MOVE_FEATHER_FALL);
#endif
        SendMessageToSet(&data, true);
    }

    if (IsCreature())
    {
        WorldPacket data(SMSG_SPLINE_MOVE_FEATHER_FALL, 9);
#if VERSION_STRING != Cata
        data << GetNewGUID();
#else
        movement_info.writeMovementInfo(data, SMSG_SPLINE_MOVE_FEATHER_FALL);
#endif
        SendMessageToSet(&data, false);
    }
}

void Unit::setMoveNormalFall()
{
    RemoveUnitMovementFlag(MOVEFLAG_FEATHER_FALL);

    if (IsPlayer())
    {
        WorldPacket data(SMSG_MOVE_NORMAL_FALL, 12);
#if VERSION_STRING != Cata
        data << GetNewGUID();
        data << uint32(0);
#else
        movement_info.writeMovementInfo(data, SMSG_MOVE_NORMAL_FALL);
#endif
        SendMessageToSet(&data, true);
    }

    if (IsCreature())
    {
        WorldPacket data(SMSG_SPLINE_MOVE_NORMAL_FALL, 9);
#if VERSION_STRING != Cata
        data << GetNewGUID();
#else
        movement_info.writeMovementInfo(data, SMSG_SPLINE_MOVE_NORMAL_FALL);
#endif
        SendMessageToSet(&data, false);
    }
}

void Unit::setMoveHover(bool set_hover)
{
    if (IsPlayer())
    {
        if (set_hover)
        {
            AddUnitMovementFlag(MOVEFLAG_HOVER);

            WorldPacket data(SMSG_MOVE_SET_HOVER, 13);
#if VERSION_STRING != Cata
            data << GetNewGUID();
            data << uint32(0);
#else
            movement_info.writeMovementInfo(data, SMSG_MOVE_SET_HOVER);
#endif
            SendMessageToSet(&data, true);
        }
        else
        {
            RemoveUnitMovementFlag(MOVEFLAG_HOVER);

            WorldPacket data(SMSG_MOVE_UNSET_HOVER, 13);
#if VERSION_STRING != Cata
            data << GetNewGUID();
            data << uint32(0);
#else
            movement_info.writeMovementInfo(data, SMSG_MOVE_UNSET_HOVER);
#endif
            SendMessageToSet(&data, true);
        }
    }

    //\todo spline update
    if (IsCreature())
    {
        if (set_hover)
        {
            AddUnitMovementFlag(MOVEFLAG_HOVER);

            setByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_HOVER);

            WorldPacket data(SMSG_SPLINE_MOVE_SET_HOVER, 10);
#if VERSION_STRING != Cata
            data << GetNewGUID();
#else
            movement_info.writeMovementInfo(data, SMSG_SPLINE_MOVE_SET_HOVER);
#endif
            SendMessageToSet(&data, false);
        }
        else
        {
            RemoveUnitMovementFlag(MOVEFLAG_HOVER);

            removeByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_HOVER);

            WorldPacket data(SMSG_SPLINE_MOVE_UNSET_HOVER, 10);
#if VERSION_STRING != Cata
            data << GetNewGUID();
#else
            movement_info.writeMovementInfo(data, SMSG_SPLINE_MOVE_UNSET_HOVER);
#endif
            SendMessageToSet(&data, false);
        }
    }
}

void Unit::setMoveCanFly(bool set_fly)
{
    if (IsPlayer())
    {
        if (set_fly)
        {
            AddUnitMovementFlag(MOVEFLAG_CAN_FLY);

            // Remove falling flag if set
            RemoveUnitMovementFlag(MOVEFLAG_FALLING);

            WorldPacket data(SMSG_MOVE_SET_CAN_FLY, 13);
#if VERSION_STRING != Cata
            data << GetNewGUID();
            data << uint32(0);
#else
            movement_info.writeMovementInfo(data, SMSG_MOVE_SET_CAN_FLY);
#endif
            SendMessageToSet(&data, true);
        }
        else
        {
            // Remove all fly related moveflags
            RemoveUnitMovementFlag(MOVEFLAG_CAN_FLY);
            RemoveUnitMovementFlag(MOVEFLAG_DESCENDING);
            RemoveUnitMovementFlag(MOVEFLAG_ASCENDING);

            WorldPacket data(SMSG_MOVE_UNSET_CAN_FLY, 13);
#if VERSION_STRING != Cata
            data << GetNewGUID();
            data << uint32(0);
#else
            movement_info.writeMovementInfo(data, SMSG_MOVE_UNSET_CAN_FLY);
#endif
            SendMessageToSet(&data, true);
        }
    }

    if (IsCreature())
    {
        if (set_fly)
        {
            AddUnitMovementFlag(MOVEFLAG_CAN_FLY);

            // Remove falling flag if set
            RemoveUnitMovementFlag(MOVEFLAG_FALLING);

            WorldPacket data(SMSG_SPLINE_MOVE_SET_FLYING, 10);
#if VERSION_STRING != Cata
            data << GetNewGUID();
#else
            movement_info.writeMovementInfo(data, SMSG_SPLINE_MOVE_SET_FLYING);
#endif
            SendMessageToSet(&data, false);
        }
        else
        {
            // Remove all fly related moveflags
            RemoveUnitMovementFlag(MOVEFLAG_CAN_FLY);
            RemoveUnitMovementFlag(MOVEFLAG_DESCENDING);
            RemoveUnitMovementFlag(MOVEFLAG_ASCENDING);

            WorldPacket data(SMSG_SPLINE_MOVE_UNSET_FLYING, 10);
#if VERSION_STRING != Cata
            data << GetNewGUID();
#else
            movement_info.writeMovementInfo(data, SMSG_SPLINE_MOVE_UNSET_FLYING);
#endif
            SendMessageToSet(&data, false);
        }
    }
}

void Unit::setMoveRoot(bool set_root)
{
    if (IsPlayer())
    {
        if (set_root)
        {
            AddUnitMovementFlag(MOVEFLAG_ROOTED);

            WorldPacket data(SMSG_FORCE_MOVE_ROOT, 12);
#if VERSION_STRING != Cata
            data << GetNewGUID();
            data << uint32(0);
#else
            movement_info.writeMovementInfo(data, SMSG_FORCE_MOVE_ROOT);
#endif
            SendMessageToSet(&data, true);
        }
        else
        {
            RemoveUnitMovementFlag(MOVEFLAG_ROOTED);

            WorldPacket data(SMSG_FORCE_MOVE_UNROOT, 12);
#if VERSION_STRING != Cata
            data << GetNewGUID();
            data << uint32(0);
#else
            movement_info.writeMovementInfo(data, SMSG_FORCE_MOVE_UNROOT);
#endif
            SendMessageToSet(&data, true);
        }
    }

    if (IsCreature())
    {
        if (set_root)
        {
            // AIInterface
            //\todo stop movement based on movement flag instead of m_canMove
            m_aiInterface->m_canMove = false;
            m_aiInterface->StopMovement(100);

            AddUnitMovementFlag(MOVEFLAG_ROOTED);

            WorldPacket data(SMSG_SPLINE_MOVE_ROOT, 9);
#if VERSION_STRING != Cata
            data << GetNewGUID();
#else
            movement_info.writeMovementInfo(data, SMSG_SPLINE_MOVE_ROOT);
#endif
            SendMessageToSet(&data, true);
        }
        else
        {
            m_aiInterface->m_canMove = true;

            RemoveUnitMovementFlag(MOVEFLAG_ROOTED);

            WorldPacket data(SMSG_SPLINE_MOVE_UNROOT, 9);
#if VERSION_STRING != Cata
            data << GetNewGUID();
#else
            movement_info.writeMovementInfo(data, SMSG_SPLINE_MOVE_UNROOT);
#endif
            SendMessageToSet(&data, true);
        }
    }
}

bool Unit::isRooted() const
{
    return HasUnitMovementFlag(MOVEFLAG_ROOTED);
}

void Unit::setMoveSwim(bool set_swim)
{
    if (IsCreature())
    {
        if (set_swim)
        {
            AddUnitMovementFlag(MOVEFLAG_SWIMMING);

            WorldPacket data(SMSG_SPLINE_MOVE_START_SWIM, 10);
#if VERSION_STRING != Cata
            data << GetNewGUID();
#else
            movement_info.writeMovementInfo(data, SMSG_SPLINE_MOVE_START_SWIM);
#endif
            SendMessageToSet(&data, false);
        }
        else
        {
            RemoveUnitMovementFlag(MOVEFLAG_SWIMMING);

            WorldPacket data(SMSG_SPLINE_MOVE_STOP_SWIM, 10);
#if VERSION_STRING != Cata
            data << GetNewGUID();
#else
            movement_info.writeMovementInfo(data, SMSG_SPLINE_MOVE_STOP_SWIM);
#endif
            SendMessageToSet(&data, false);
        }
    }
}

void Unit::setMoveDisableGravity(bool disable_gravity)
{
#if VERSION_STRING > TBC
    if (IsPlayer())
    {
        if (disable_gravity)
        {
            AddUnitMovementFlag(MOVEFLAG_DISABLEGRAVITY);

            WorldPacket data(SMSG_MOVE_GRAVITY_DISABLE, 13);
#if VERSION_STRING != Cata
            data << GetNewGUID();
            data << uint32(0);
#else
            movement_info.writeMovementInfo(data, SMSG_MOVE_GRAVITY_DISABLE);
#endif
            SendMessageToSet(&data, true);
        }
        else
        {
            RemoveUnitMovementFlag(MOVEFLAG_DISABLEGRAVITY);

            WorldPacket data(SMSG_MOVE_GRAVITY_ENABLE, 13);
#if VERSION_STRING != Cata
            data << GetNewGUID();
            data << uint32(0);
#else
            movement_info.writeMovementInfo(data, SMSG_MOVE_GRAVITY_ENABLE);
#endif
            SendMessageToSet(&data, true);
        }
    }

    if (IsCreature())
    {
        if (disable_gravity)
        {
            AddUnitMovementFlag(MOVEFLAG_DISABLEGRAVITY);

            WorldPacket data(SMSG_SPLINE_MOVE_GRAVITY_DISABLE, 10);
#if VERSION_STRING != Cata
            data << GetNewGUID();
#else
            movement_info.writeMovementInfo(data, SMSG_SPLINE_MOVE_GRAVITY_DISABLE);
#endif
            SendMessageToSet(&data, false);
        }
        else
        {
            RemoveUnitMovementFlag(MOVEFLAG_DISABLEGRAVITY);

            WorldPacket data(SMSG_SPLINE_MOVE_GRAVITY_ENABLE, 10);
#if VERSION_STRING != Cata
            data << GetNewGUID();
#else
            movement_info.writeMovementInfo(data, SMSG_SPLINE_MOVE_GRAVITY_ENABLE);
#endif
            SendMessageToSet(&data, false);
        }
    }
#endif
}

//\todo Zyres: call it if creature has MoveFlag in its movement info (set in Object::_BuildMovementUpdate)
//             Unfortunately Movement and object update is a mess.
void Unit::setMoveWalk(bool set_walk)
{
    if (IsCreature())
    {
        if (set_walk)
        {
            AddUnitMovementFlag(MOVEFLAG_WALK);

            WorldPacket data(SMSG_SPLINE_MOVE_SET_WALK_MODE, 10);
#if VERSION_STRING != Cata
            data << GetNewGUID();
#else
            movement_info.writeMovementInfo(data, SMSG_SPLINE_MOVE_SET_WALK_MODE);
#endif
            SendMessageToSet(&data, false);
        }
        else
        {
            RemoveUnitMovementFlag(MOVEFLAG_WALK);

            WorldPacket data(SMSG_SPLINE_MOVE_SET_RUN_MODE, 10);
#if VERSION_STRING != Cata
            data << GetNewGUID();
#else
            movement_info.writeMovementInfo(data, SMSG_SPLINE_MOVE_SET_RUN_MODE);
#endif
            SendMessageToSet(&data, false);
        }
    }
}

float Unit::getSpeedForType(UnitSpeedType speed_type, bool get_basic)
{
    switch (speed_type)
    {
        case TYPE_WALK:
            return (get_basic ? m_basicSpeedWalk : m_currentSpeedWalk);
        case TYPE_RUN:
            return (get_basic ? m_basicSpeedRun : m_currentSpeedRun);
        case TYPE_RUN_BACK:
            return (get_basic ? m_basicSpeedRunBack : m_currentSpeedRunBack);
        case TYPE_SWIM:
            return (get_basic ? m_basicSpeedSwim : m_currentSpeedSwim);
        case TYPE_SWIM_BACK:
            return (get_basic ? m_basicSpeedSwimBack : m_currentSpeedSwimBack);
        case TYPE_TURN_RATE:
            return (get_basic ? m_basicTurnRate : m_currentTurnRate);
        case TYPE_FLY:
            return (get_basic ? m_basicSpeedFly : m_currentSpeedFly);
        case TYPE_FLY_BACK:
            return (get_basic ? m_basicSpeedFlyBack : m_currentSpeedFlyBack);
        case TYPE_PITCH_RATE:
            return (get_basic ? m_basicPitchRate : m_currentPitchRate);
        default:
            return m_basicSpeedWalk;
    }
}

void Unit::setSpeedForType(UnitSpeedType speed_type, float speed, bool set_basic)
{
    switch (speed_type)
    {
        case TYPE_WALK:
        {
            if (set_basic)
                m_basicSpeedWalk = speed;
            else
                m_currentSpeedWalk = speed;
        } break;
        case TYPE_RUN:
        {
            if (set_basic)
                m_basicSpeedRun = speed;
            else
                m_currentSpeedRun = speed;
        } break;
        case TYPE_RUN_BACK:
        {
            if (set_basic)
                m_basicSpeedRunBack = speed;
            else
                m_currentSpeedRunBack = speed;
        } break;
        case TYPE_SWIM:
        {
            if (set_basic)
                m_basicSpeedSwim = speed;
            else
                m_currentSpeedSwim = speed;
        } break;
        case TYPE_SWIM_BACK:
        {
            if (set_basic)
                m_basicSpeedSwimBack = speed;
            else
                m_currentSpeedSwimBack = speed;
        } break;
        case TYPE_TURN_RATE:
        {
            if (set_basic)
                m_basicTurnRate = speed;
            else
                m_currentTurnRate = speed;
        } break;
        case TYPE_FLY:
        {
            if (set_basic)
                 m_basicSpeedFly = speed;
            else
                m_currentSpeedFly = speed;
        } break;
        case TYPE_FLY_BACK:
        {
            if (set_basic)
                m_basicSpeedFlyBack = speed;
            else
                m_currentSpeedFlyBack = speed;
        } break;
        case TYPE_PITCH_RATE:
        {
            if (set_basic)
                m_basicPitchRate = speed;
            else
                m_currentPitchRate = speed;
        } break;
    }

    Player* player_mover = GetMapMgrPlayer(GetCharmedByGUID());
    if (player_mover == nullptr)
    {
        if (IsPlayer())
            player_mover = static_cast<Player*>(this);
    }

    if (player_mover != nullptr)
    {
#if VERSION_STRING != Cata
        player_mover->sendForceMovePacket(speed_type, speed);
#endif
        player_mover->sendMoveSetSpeedPaket(speed_type, speed);
    }
    else
    {
        sendMoveSplinePaket(speed_type);
    }

}

void Unit::resetCurrentSpeed()
{
    m_currentSpeedWalk = m_basicSpeedWalk;
    m_currentSpeedRun = m_basicSpeedRun;
    m_currentSpeedRunBack = m_basicSpeedRunBack;
    m_currentSpeedSwim = m_basicSpeedSwim;
    m_currentSpeedSwimBack = m_basicSpeedSwimBack;
    m_currentTurnRate = m_basicTurnRate;
    m_currentSpeedFly = m_basicSpeedFly;
    m_currentSpeedFlyBack = m_basicSpeedFlyBack;
    m_currentPitchRate = m_basicPitchRate;
}

void Unit::sendMoveSplinePaket(UnitSpeedType speed_type)
{
    WorldPacket data(12);

    switch (speed_type)
    {
        case TYPE_WALK:
            data.Initialize(SMSG_SPLINE_SET_WALK_SPEED);
            break;
        case TYPE_RUN:
            data.Initialize(SMSG_SPLINE_SET_RUN_SPEED);
            break;
        case TYPE_RUN_BACK:
            data.Initialize(SMSG_SPLINE_SET_RUN_BACK_SPEED);
            break;
        case TYPE_SWIM:
            data.Initialize(SMSG_SPLINE_SET_SWIM_SPEED);
            break;
        case TYPE_SWIM_BACK:
            data.Initialize(SMSG_SPLINE_SET_SWIM_BACK_SPEED);
            break;
        case TYPE_TURN_RATE:
            data.Initialize(SMSG_SPLINE_SET_TURN_RATE);
            break;
        case TYPE_FLY:
            data.Initialize(SMSG_SPLINE_SET_FLIGHT_SPEED);
            break;
        case TYPE_FLY_BACK:
            data.Initialize(SMSG_SPLINE_SET_FLIGHT_BACK_SPEED);
            break;
#if VERSION_STRING > TBC
        case TYPE_PITCH_RATE:
            data.Initialize(SMSG_SPLINE_SET_PITCH_RATE);
            break;
#endif
    }

    data << GetNewGUID();
    data << float(getSpeedForType(speed_type));

    SendMessageToSet(&data, false);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Spells

#ifdef USE_EXPERIMENTAL_SPELL_SYSTEM
bool Unit::isCastingNonMeleeSpell(bool checkDelayed, bool checkChanneled, bool checkAutoRepeat, bool isAutoShoot, bool checkInstantSpells) const
{
    // First we check non-channeled spells, not the best way to check is spell a channeled spell, but the only way right now
    if (m_currentSpell && m_currentSpell->GetSpellInfo()->ChannelInterruptFlags == 0 &&
        m_currentSpell->getState() != SPELL_STATE_FINISHED && (checkDelayed || m_currentSpell->getState() != SPELL_STATE_DELAYED))
    {
        if (!checkInstantSpells || m_currentSpell->getCastTime())
        {
            if (!isAutoShoot || !m_currentSpell->GetSpellInfo()->hasAttributes(ATTRIBUTESEXB_DONT_RESET_AUTO_ACTIONS))
                return true;
        }
    }

    // then we check channeled spells
    if (!checkChanneled && m_currentSpell && m_currentSpell->GetSpellInfo()->ChannelInterruptFlags != 0 &&
        m_currentSpell->getState() != SPELL_STATE_FINISHED)
    {
        if (!isAutoShoot || !m_currentSpell->GetSpellInfo()->hasAttributes(ATTRIBUTESEXB_DONT_RESET_AUTO_ACTIONS))
            return true;
    }

    // and last we check auto repeat spells
    if (!checkAutoRepeat && m_currentSpell && m_currentSpell->GetSpellInfo()->hasAttributes(ATTRIBUTESEXB_AUTO_REPEAT))
        return true;

    return false;
}
#endif

void Unit::removeAurasWithModType(AuraModTypes type)
{
    for (uint32_t i = MAX_TOTAL_AURAS_START; i < MAX_TOTAL_AURAS_END; ++i)
    {
        if (m_auras[i] == nullptr)
            continue;
        if (m_auras[i]->HasModType(type))
            RemoveAura(m_auras[i]);
    }
}

bool Unit::hasAuraWithAuraType(AuraModTypes type)
{
    bool found = false;
    for (uint32_t i = MAX_TOTAL_AURAS_START; i < MAX_TOTAL_AURAS_END; ++i)
    {
        if (m_auras[i] == nullptr)
            continue;

        if (m_auras[i]->HasModType(type))
        {
            found = true;
            break;
        }
    }
    return found;
}

bool Unit::hasAuraState(AuraStates state, SpellInfo const* spellInfo, Unit* caster) const
{
    if (caster && spellInfo)
    {
        for (uint32_t i = MAX_TOTAL_AURAS_START; i < MAX_TOTAL_AURAS_END; ++i)
        {
            if (!caster->m_auras[i])
                continue;
            if (!caster->m_auras[i]->HasModType(SPELL_AURA_IGNORE_TARGET_AURA_STATE))
                continue;
            if (spellInfo->isAffectedBySpell(caster->m_auras[i]->GetSpellInfo()))
                return true;
        }
    }
    return HasFlag(UNIT_FIELD_AURASTATE, 1 << (state - 1));
}

void Unit::modifyAuraState(AuraStates state, bool apply)
{
    if (apply)
    {
        if (!HasFlag(UNIT_FIELD_AURASTATE, 1 << (state - 1)))
        {
            SetFlag(UNIT_FIELD_AURASTATE, 1 << (state - 1));
            if (IsPlayer())
            {
                // Cast spells on player which require this aura state
                SpellSet const& playerSpellMap = ((Player*)this)->mSpells;
                for (auto itr : playerSpellMap)
                {
                    // If player has removed this spell or the spell is disabled, skip it
                    SpellSet::const_iterator iter = ((Player*)this)->mDeletedSpells.find(itr);
                    if ((iter != ((Player*)this)->mDeletedSpells.end()) || objmgr.IsSpellDisabled(itr))
                        continue;
                    SpellInfo const*spellInfo = sSpellCustomizations.GetSpellInfo(itr);
                    if (!spellInfo || spellInfo->IsPassive())
                        continue;
                    if (AuraStates(spellInfo->CasterAuraState) == state)
                        CastSpell(this, itr, true);
                }
            }
        }
    }
    else
    {
        if (HasFlag(UNIT_FIELD_AURASTATE, 1 << (state - 1)))
        {
            RemoveFlag(UNIT_FIELD_AURASTATE, 1 << (state - 1));

            if (state != AURASTATE_FLAG_ENRAGE)
            {
                for (uint32_t i = MAX_TOTAL_AURAS_START; i < MAX_TOTAL_AURAS_END; ++i)
                {
                    if (m_auras[i] == nullptr || !m_auras[i]->m_spellInfo)
                        continue;
                    if (AuraStates(m_auras[i]->m_spellInfo->CasterAuraState) == state)
                        RemoveAura(m_auras[i]);
                }
            }
        }
    }
}

bool Unit::canSeeOrDetect(Object* obj, bool checkStealth)
{
    if (!obj)
        return false;

    if (this == obj)
        return true;

    if (!obj->IsInWorld() || (GetMapId() != obj->GetMapId() || GetPhase() != obj->GetPhase()))
        return false;

    // If the object is invisible Game Master, we do not have to check further
    // TODO: should GMs be able to see other invisible GMs?
    if (obj->IsPlayer() && static_cast<Player*>(obj)->m_isGmInvisible)
        return false;

    // We are dead and we have released the spirit
    if (IsPlayer() && getDeathState() == CORPSE)
    {
        Player* playerMe = static_cast<Player*>(this);
        const float corpseViewDistance = 1600.0f; // 40*40 yards
        // If the object is a player
        if (obj->IsPlayer())
        {
            Player* playerTarget = static_cast<Player*>(obj);
            if (playerMe->getMyCorpseInstanceId() == playerMe->GetInstanceID() &&
                playerTarget->getDistanceSq(playerMe->getMyCorpseLocation()) <= corpseViewDistance)
                // We can see all players within range of our corpse
                return true;

            // We can see all players in arena
            if (playerMe->m_deathVision)
                return true;

            // Otherwise we can only see players who have released their spirits as well
            return (playerTarget->getDeathState() == CORPSE);
        }

        if (playerMe->getMyCorpseInstanceId() == playerMe->GetInstanceID())
        {
            // We can see our corpse
            if (obj->IsCorpse() && static_cast<Corpse*>(obj)->GetOwner() == playerMe->GetGUID())
                return true;

            // We can see everything within range of our corpse
            if (obj->getDistanceSq(playerMe->getMyCorpseLocation()) <= corpseViewDistance)
                return true;
        }

        // We can see everything in arena
        if (playerMe->m_deathVision)
            return true;

        // We can see spirit healers
        if (obj->IsCreature() && static_cast<Creature*>(obj)->isSpiritHealer())
            return true;

        return false;
    }

    Unit* unitTarget = static_cast<Unit*>(obj);
    GameObject* gobTarget = static_cast<GameObject*>(obj);

    // We are alive or we haven't released the spirit yet
    switch (obj->GetTypeId())
    {
        case TYPEID_PLAYER:
        {
            Player* playerTarget = static_cast<Player*>(obj);
            if (IsPlayer())
            {
                // If player unit has released his/her spirit
                if (playerTarget->getDeathState() == CORPSE)
                {
                    // If the target is in same group or raid
                    if (static_cast<Player*>(this)->GetGroup() && static_cast<Player*>(this)->GetGroup() == playerTarget->GetGroup())
                        return true;
                    // otherwise only Game Master can see the player
                    return (HasFlag(PLAYER_FLAGS, PLAYER_FLAG_GM) != 0);
                }
            }
            break;
        }
        case TYPEID_UNIT:
        {
            // Can't see spirit healers when alive
            // but Game Masters can
            if (unitTarget->IsCreature() && static_cast<Creature*>(unitTarget)->isSpiritHealer())
                return (HasFlag(PLAYER_FLAGS, PLAYER_FLAG_GM) != 0);

            // We can always see our own units
            if (GetGUID() == unitTarget->GetCreatedByGUID())
                return true;

            // If unit is visible only to other faction
            if (IsPlayer())
            {
                if (unitTarget->GetAIInterface()->faction_visibility == 1)
                    return static_cast<Player*>(this)->IsTeamHorde() ? true : false;

                if (unitTarget->GetAIInterface()->faction_visibility == 2)
                    return static_cast<Player*>(this)->IsTeamHorde() ? false : true;
            }
            break;
        }
        case TYPEID_GAMEOBJECT:
        {
            if (gobTarget->isStealthed || gobTarget->isInvisible)
            {
                uint64_t ownerGuid = gobTarget->getUInt64Value(OBJECT_FIELD_CREATED_BY);
                // We can always see our game objects (i.e hunter traps)
                if (GetGUID() == ownerGuid)
                    return true;

                // Group and raid members can see our game objects as well
                // expect if we are dueling them
                if (IsPlayer() && static_cast<Player*>(this)->GetGroup())
                {
                    Player* ownerPlayer = GetMapMgrPlayer((uint32)ownerGuid);
                    if (ownerPlayer && static_cast<Player*>(this)->GetGroup()->HasMember(ownerPlayer) &&
                        static_cast<Player*>(this)->DuelingWith != ownerPlayer)
                        return true;
                }
            }
            break;
        }
        default:
            break;
    }

    // Game Masters can see invisible and stealthed objects
    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAG_GM))
        return true;

    // Pets don't have detection, they rely on their master's detection
    Unit* meUnit = this;
    if (GetSummonedByGUID() != 0)
    {
        if (Unit* summoner = GetMapMgrUnit(GetSummonedByGUID()))
            meUnit = summoner;
    }
    if (GetCharmedByGUID() != 0)
    {
        if (Unit* charmer = GetMapMgrUnit(GetCharmedByGUID()))
            meUnit = charmer;
    }

    // Hunter's Marked units are always visible to caster
    if (obj->IsUnit() && unitTarget->stalkedby == meUnit->GetGUID())
        return true;

    if (meUnit->IsPlayer())
    {
        // If object, or its summoner/owner, is in same group with us, we can always see it
        // expect if we are dueling it
        Unit* objOwner = obj->IsUnit() ? unitTarget : nullptr;
        if (obj->IsUnit())
        {
            if (obj->getUInt64Value(UNIT_FIELD_SUMMONEDBY) != 0)
            {
                if (Unit* objSummoner = obj->GetMapMgrUnit(getUInt64Value(UNIT_FIELD_SUMMONEDBY)))
                    objOwner = objSummoner;
            }
            if (obj->getUInt64Value(UNIT_FIELD_CHARMEDBY) != 0)
            {
                if (Unit* objCharmer = obj->GetMapMgrUnit(getUInt64Value(UNIT_FIELD_CHARMEDBY)))
                    objOwner = objCharmer;
            }
        }
        else if (obj->IsGameObject())
        {
            if (obj->getUInt64Value(OBJECT_FIELD_CREATED_BY) != 0)
            {
                if (Unit* objSummoner = obj->GetMapMgrUnit(getUInt64Value(OBJECT_FIELD_CREATED_BY)))
                    objOwner = objSummoner;
            }
        }

        if (objOwner && objOwner->IsPlayer())
        {
            if (static_cast<Player*>(meUnit)->GetGroup() && static_cast<Player*>(meUnit)->GetGroup()->HasMember(static_cast<Player*>(objOwner)) &&
                static_cast<Player*>(meUnit)->DuelingWith != static_cast<Player*>(objOwner))
                return true;
        }
    }

    // Invisibility Detection
    for (int i = 0; i < INVIS_FLAG_TOTAL; ++i)
    {
        int32_t ourInvisibilityValue = meUnit->getInvisibilityLevel(InvisibilityFlag(i)) ? meUnit->getInvisibilityLevel(InvisibilityFlag(i)) : 0;
        int32_t ourInvisibilityDetection = meUnit->getInvisibilityDetectBonus(InvisibilityFlag(i)) ? meUnit->getInvisibilityDetectBonus(InvisibilityFlag(i)) : 0;
        int32_t objectInvisibilityValue = 0;
        int32_t objectInvisibilityDetection = 0;
        if (obj->IsUnit())
        {
            objectInvisibilityValue = unitTarget->getInvisibilityLevel(InvisibilityFlag(i)) ? unitTarget->getInvisibilityLevel(InvisibilityFlag(i)) : 0;
            objectInvisibilityDetection = unitTarget->getInvisibilityDetectBonus(InvisibilityFlag(i)) ? unitTarget->getInvisibilityDetectBonus(InvisibilityFlag(i)) : 0;

            // When we are invisible we can only see those who have enough detection value to see us
            // When object is invisible we can only see it if we have enough detection value
            if ((ourInvisibilityValue > objectInvisibilityDetection) ||
                (objectInvisibilityValue > ourInvisibilityDetection))
                return false;
        }
        else if (obj->IsGameObject() && i == INVIS_FLAG_TRAP)
        {
            // TODO: implement game object invisibility (yes, there are stealthed and invisible game objects)
            // currently only stealthed game objects are implemented

            /*if (!gobTarget->isInvisible)
                continue;

            objectsInvisibilityValue = gobTarget->invisibilityValue ? gobTarget->invisibilityValue : 0;
            if (objectsInvisibilityValue > ourInvisibilityDetection)
                return false;*/
        }
    }

    if (!checkStealth)
        return true;

    // Stealth Detection
    // TODO: may need some tweaking...
    if ((obj->IsUnit() && unitTarget->isStealthed()) || (obj->IsGameObject() && gobTarget->isStealthed))
    {
        const float distance = meUnit->getDistanceSq(obj);
        const float combatReach = meUnit->GetCombatReach();

        // If object is closer than our melee range
        if (distance < combatReach)
            return true;

        if (obj->IsUnit())
        {
            // Some spells make you detect stealth regardless of range and facing
            // i.e Shadow Sight buff in arena
            if (meUnit->hasAuraWithAuraType(SPELL_AURA_DETECT_STEALTH))
                return true;

            // We can't normally detect units outside line of sight
            if (worldConfig.terrainCollision.isCollisionEnabled)
            {
                if (!meUnit->IsWithinLOSInMap(obj))
                    return false;
            }

            // We can't normally detect what is not in front of us
            if (!meUnit->isInFront(obj))
                return false;
        }

        // In unit cases base stealth level and base stealth detection increases by 5 points per unit level, starting from level 1
        // Gameobject traps seems to have 70 base stealth value and they inherit higher stealth value from their owner, if they have one
        int32_t detectionValue = meUnit->getLevel() * 5;

        // Apply modifiers which increase stealth detection
        detectionValue += obj->IsUnit() ? meUnit->getStealthDetectBonus(STEALTH_FLAG_NORMAL) : meUnit->getStealthDetectBonus(STEALTH_FLAG_TRAP);

        // Subtract object's unit level and stealth level
        if (obj->IsGameObject())
        {
            detectionValue -= gobTarget->stealthValue;
            if (gobTarget->m_summoner && gobTarget->m_summoner->IsInWorld())
                detectionValue -= gobTarget->m_summoner->getLevel() * 5;
        }
        else
            detectionValue -= unitTarget->getLevel() * 5 + unitTarget->getStealthLevel(STEALTH_FLAG_NORMAL);

        float visibilityRange = float(detectionValue) * 0.3f + combatReach;
        if (visibilityRange <= 0.0f)
            return false;
        // Players cannot see stealthed objects from further than 30 yards
        if (visibilityRange > 30.0f && meUnit->IsPlayer())
            visibilityRange = 30.0f;

        if (distance > visibilityRange)
            return false;
    }
    return true;
}

void Unit::playSpellVisual(uint64_t guid, uint32_t spell_id)
{
#if VERSION_STRING != Cata
    WorldPacket data(SMSG_PLAY_SPELL_VISUAL, 12);
    data << uint64_t(guid);
    data << uint32_t(spell_id);

    if (IsPlayer())
        static_cast<Player*>(this)->SendMessageToSet(&data, true);
    else
        SendMessageToSet(&data, false);
#else
    WorldPacket data(SMSG_PLAY_SPELL_VISUAL, 8 + 4 + 8);
    data << uint32_t(0);
    data << uint32_t(spell_id);

    data << uint32_t(guid == GetGUID() ? 1 : 0);

    ObjectGuid targetGuid = guid;
    data.writeBit(targetGuid[4]);
    data.writeBit(targetGuid[7]);
    data.writeBit(targetGuid[5]);
    data.writeBit(targetGuid[3]);
    data.writeBit(targetGuid[1]);
    data.writeBit(targetGuid[2]);
    data.writeBit(targetGuid[0]);
    data.writeBit(targetGuid[6]);

    data.flushBits();

    data.WriteByteSeq(targetGuid[0]);
    data.WriteByteSeq(targetGuid[4]);
    data.WriteByteSeq(targetGuid[1]);
    data.WriteByteSeq(targetGuid[6]);
    data.WriteByteSeq(targetGuid[7]);
    data.WriteByteSeq(targetGuid[2]);
    data.WriteByteSeq(targetGuid[3]);
    data.WriteByteSeq(targetGuid[5]);

    SendMessageToSet(&data, true);
#endif
}

void Unit::applyDiminishingReturnTimer(uint32_t* duration, SpellInfo const* spell)
{
    uint32_t status = spell->custom_DiminishStatus;
    uint32_t group  = status & 0xFFFF;
    uint32_t PvE    = (status >> 16) & 0xFFFF;

    // Make sure we have a group
    if (group == 0xFFFF)
    {
        return;
    }

    // Check if we don't apply to pve
    if (!PvE && !IsPlayer() && !IsPet())
    {
        return;
    }

    uint32_t localDuration = *duration;
    uint32_t count = m_diminishCount[group];

    // Target immune to spell
    if (count > 2)
    {
        *duration = 0;
        return;
    }

    //100%, 50%, 25% bitwise
    localDuration >>= count;
    if ((IsPlayer() || IsPet()) && localDuration > uint32_t(10000 >> count))
    {
        localDuration = 10000 >> count;
        if (status == DIMINISHING_GROUP_NOT_DIMINISHED)
        {
            *duration = localDuration;
            return;
        }
    }

    *duration = localDuration;

    // Reset the diminishing return counter, and add to the aura count (we don't decrease the timer till we
    // have no auras of this type left.
    ++m_diminishCount[group];
}

void Unit::removeDiminishingReturnTimer(SpellInfo const* spell)
{
    uint32_t status = spell->custom_DiminishStatus;
    uint32_t group  = status & 0xFFFF;
    uint32_t pve    = (status >> 16) & 0xFFFF;
    uint32_t aura_group;

    // Make sure we have a group
    if (group == 0xFFFF)
    {
        return;
    }

    // Check if we don't apply to pve
    if (!pve && !IsPlayer() && !IsPet())
    {
        return;
    }

    /*There are cases in which you just refresh an aura duration instead of the whole aura,
    causing corruption on the diminishAura counter and locking the entire diminishing group.
    So it's better to check the active auras one by one*/
    m_diminishAuraCount[group] = 0;
    for (uint32_t x = MAX_NEGATIVE_AURAS_EXTEDED_START; x < MAX_NEGATIVE_AURAS_EXTEDED_END; ++x)
    {
        if (m_auras[x])
        {
            aura_group = m_auras[x]->GetSpellInfo()->custom_DiminishStatus;
            if (aura_group == status)
            {
                m_diminishAuraCount[group]++;
            }
        }
    }

    // Start timer decrease
    if (!m_diminishAuraCount[group])
    {
        m_diminishActive = true;
        m_diminishTimer[group] = 15000;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// Aura

Aura* Unit::getAuraWithId(uint32_t spell_id)
{
    for (uint32_t i = MAX_TOTAL_AURAS_START; i < MAX_TOTAL_AURAS_END; ++i)
    {
        Aura* aura = m_auras[i];
        if (aura != nullptr)
        {
            if (aura->GetSpellId() == spell_id)
                return aura;
        }
    }

    return nullptr;
}

Aura* Unit::getAuraWithIdForGuid(uint32_t spell_id, uint64_t target_guid)
{
    for (uint32_t i = MAX_TOTAL_AURAS_START; i < MAX_TOTAL_AURAS_END; ++i)
    {
        Aura* aura = m_auras[i];
        if (aura != nullptr)
        {
            if (getAuraWithId(spell_id) && aura->m_casterGuid == target_guid)
                return aura;
        }
    }

    return nullptr;
}

Aura* Unit::getAuraWithAuraEffect(uint32_t aura_effect)
{
    for (uint32_t i = MAX_TOTAL_AURAS_START; i < MAX_TOTAL_AURAS_END; ++i)
    {
        Aura* aura = m_auras[i];
        if (aura != nullptr && aura->GetSpellInfo()->HasEffectApplyAuraName(aura_effect))
            return aura;
    }

    return nullptr;
}
