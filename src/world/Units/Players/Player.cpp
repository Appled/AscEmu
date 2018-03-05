/*
Copyright (c) 2014-2018 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#include "StdAfx.h"

#include "Player.h"
#include "Server/Packets/Opcode.h"
#include "Chat/ChatDefines.hpp"
#include "Server/World.h"
#include "Spell/Definitions/PowerType.h"
#include "Spell/Spell.h"
#include "Spell/SpellMgr.h"
#include "Spell/SpellFailure.h"
#include "Map/MapMgr.h"
#include "Data/WoWPlayer.h"
#include "Management/Battleground/Battleground.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Data
bool Player::createPlayer(WorldPacket& playerCreateData)
{
    // TODO: move race checks and class checks and char limits to characterhandler from legacy player create
    // need to rewrite char create opcode as well then

    uint8_t race, _class, gender, skinColor, face, outfitId, hairStyle, hairColor, facialHair;

    playerCreateData >> m_name >> race >> _class >> gender >> skinColor >> face >> hairStyle >> hairColor >> facialHair >> outfitId;
    Util::CapitalizeString(m_name);

    // Load player create info
    info = sMySQLStore.getPlayerCreateInfo(race, _class);
    if (info == nullptr)
    {
        LOG_ERROR("Player::createPlayer Account Id %u tried to create a character with invalid race/class combination. If this is intended, please update your playercreateinfo table.", m_session->GetAccountId());
        return false;
    }

    // Set position
    SetMapId(info->mapId);
    SetZoneId(info->zoneId);
    m_position.ChangeCoords(info->positionX, info->positionY, info->positionZ);
    m_bind_pos_x = info->positionX, m_bind_pos_y = info->positionY, m_bind_pos_z = info->positionZ, m_bind_mapid = info->mapId, m_bind_zoneid = info->zoneId;

    // Load DBC data for race and class
    myRace = sChrRacesStore.LookupEntry(race);
    myClass = sChrClassesStore.LookupEntry(_class);
    if (myRace == nullptr || myClass == nullptr)
    {
        LOG_ERROR("Player::createPlayer Account Id %u tried to create a character with invalid race or class.", m_session->GetAccountId());
        return false;
    }

    // Faction, race and class
    if (myRace->team_id == 7)
        m_team = 0; // Alliance
    else
        m_team = 1; // Horde
    setRace(race);
    setClass(_class);
    setGender(gender);
    SetFaction(myRace->faction_id);
    setScaleX(1.0f);
    m_cache->SetUInt32Value(CACHE_PLAYER_INITIALTEAM, m_team);
    if (race != RACE_BLOODELF)
    {
        SetDisplayId(info->displayId + gender);
        SetNativeDisplayId(info->displayId + gender);
    }
    else
    {
        SetDisplayId(info->displayId - gender);
        SetNativeDisplayId(info->displayId - gender);
    }

    uint8_t powerType = static_cast<uint8_t>(myClass->power_type);
    setPowerType(powerType);

    // Starting level and talent points
    uint32_t startLevel = worldConfig.player.playerStartingLevel;
    uint32_t talentPoints = startLevel >= 10 ? startLevel - 9 : 0;
#if VERSION_STRING >= WotLK
    if (_class == DEATHKNIGHT)
    {
        startLevel = std::max(55, worldConfig.player.playerStartingLevel);
        talentPoints = worldConfig.player.deathKnightStartTalentPoints;
    }
#endif
    setLevel(startLevel);
    SetTalentPointsForAllSpec(talentPoints);

#if VERSION_STRING >= TBC
    write(playerData()->field_max_level, worldConfig.player.playerLevelCap);
#endif
    SetCastSpeedMod(1.0f);
    // Load stats from level info
    LevelInfo const* levelInfo = objmgr.GetLevelInfo(race, _class, startLevel);
    if (levelInfo == nullptr)
    {
        LOG_ERROR("Player::createPlayer Account Id %u tried to create a character but level stats for race %u, class %u and level %u could not be found.", m_session->GetAccountId(), race, _class, startLevel);
        return false;
    }
    // Initial stats
    for (uint8_t i = 0; i < 5; ++i)
    {
        SetStat(STAT_STRENGTH + i, levelInfo->Stat[i]);
    }
    setBaseHealth(levelInfo->HP);
    setBaseMana(levelInfo->Mana);
    SetMaxPower(POWER_TYPE_MANA, levelInfo->Mana);
    SetMaxPower(POWER_TYPE_RAGE, 1000);
    // TODO: in cata hunters use focus instead of mana?
    //SetMaxPower(POWER_TYPE_FOCUS, ???);
    SetMaxPower(POWER_TYPE_ENERGY, 100);
    setMaxHealth(levelInfo->HP);

    // Apply health and power
    setHealth(GetMaxHealth());
    SetPower(POWER_TYPE_MANA, GetMaxPower(POWER_TYPE_MANA));
    SetPower(POWER_TYPE_ENERGY, GetMaxPower(POWER_TYPE_ENERGY));
    SetPower(POWER_TYPE_RAGE, 0);
    //SetPower(POWER_TYPE_FOCUS, ???);
#if VERSION_STRING >= WotLK
    if (getPowerType() == POWER_TYPE_RUNIC_POWER)
    {
        SetMaxPower(POWER_TYPE_RUNES, 8);
        SetMaxPower(POWER_TYPE_RUNIC_POWER, 1000);
        SetPower(POWER_TYPE_RUNES, 8);
        SetPower(POWER_TYPE_RUNIC_POWER, 0);
    }
#endif

    setPvpFlags(getPvpFlags() | U_FIELD_BYTES_FLAG_PVP);
    addUnitFlags(UNIT_FLAG_PVP_ATTACKABLE);
    addUnitFlags2(UNIT_FLAG2_ENABLE_POWER_REGEN);

    // todo: confirm level data in Stats.cpp GainStat()

    return true;
}

void Player::setAttackPowerMultiplier(float val)
{
    write(playerData()->attack_power_multiplier, val);
}

void Player::setRangedAttackPowerMultiplier(float val)
{
    write(playerData()->ranged_attack_power_multiplier, val);
}

void Player::setExploredZone(uint32_t idx, uint32_t data)
{
    ARCEMU_ASSERT(idx < WOWPLAYER_EXPLORED_ZONES_COUNT)

    write(playerData()->explored_zones[idx], data);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Movement

#if VERSION_STRING != Cata
void Player::sendForceMovePacket(UnitSpeedType speed_type, float speed)
{
    WorldPacket data(50);

    switch (speed_type)
    {
        case TYPE_WALK:
            data.Initialize(SMSG_FORCE_WALK_SPEED_CHANGE);
            break;
        case TYPE_RUN:
            data.Initialize(SMSG_FORCE_RUN_SPEED_CHANGE);
            break;
        case TYPE_RUN_BACK:
            data.Initialize(SMSG_FORCE_RUN_BACK_SPEED_CHANGE);
            break;
        case TYPE_SWIM:
            data.Initialize(SMSG_FORCE_SWIM_SPEED_CHANGE);
            break;
        case TYPE_SWIM_BACK:
            data.Initialize(SMSG_FORCE_SWIM_BACK_SPEED_CHANGE);
            break;
        case TYPE_TURN_RATE:
            data.Initialize(SMSG_FORCE_TURN_RATE_CHANGE);
            break;
        case TYPE_FLY:
            data.Initialize(SMSG_FORCE_FLIGHT_SPEED_CHANGE);
            break;
        case TYPE_FLY_BACK:
            data.Initialize(SMSG_FORCE_FLIGHT_BACK_SPEED_CHANGE);
            break;
#if VERSION_STRING > TBC
        case TYPE_PITCH_RATE:
            data.Initialize(SMSG_FORCE_PITCH_RATE_CHANGE);
            break;
#endif
    }

    data << GetNewGUID();
    data << m_speedChangeCounter++;

    if (speed_type == TYPE_RUN)
        data << uint8_t(1);

    data << float(speed);

    SendMessageToSet(&data, true);
}

void Player::sendMoveSetSpeedPaket(UnitSpeedType speed_type, float speed)
{
    WorldPacket data(45);

    switch (speed_type)
    {
#if VERSION_STRING >= WotLK
        case TYPE_WALK:
            data.Initialize(MSG_MOVE_SET_WALK_SPEED);
            break;
        case TYPE_RUN:
            data.Initialize(MSG_MOVE_SET_RUN_SPEED);
            break;
        case TYPE_RUN_BACK:
            data.Initialize(MSG_MOVE_SET_RUN_BACK_SPEED);
            break;
        case TYPE_SWIM:
            data.Initialize(MSG_MOVE_SET_SWIM_SPEED);
            break;
        case TYPE_SWIM_BACK:
            data.Initialize(MSG_MOVE_SET_SWIM_BACK_SPEED);
            break;
        case TYPE_TURN_RATE:
            data.Initialize(MSG_MOVE_SET_TURN_RATE);
            break;
        case TYPE_FLY:
            data.Initialize(MSG_MOVE_SET_FLIGHT_SPEED);
            break;
        case TYPE_FLY_BACK:
            data.Initialize(MSG_MOVE_SET_FLIGHT_BACK_SPEED);
            break;
        case TYPE_PITCH_RATE:
            data.Initialize(MSG_MOVE_SET_PITCH_RATE);
            break;
#else
        case TYPE_RUN:
            data.Initialize(SMSG_FORCE_RUN_SPEED_CHANGE);
            break;
        case TYPE_RUN_BACK:
            data.Initialize(SMSG_FORCE_RUN_BACK_SPEED_CHANGE);
            break;
        case TYPE_SWIM:
            data.Initialize(SMSG_FORCE_SWIM_SPEED_CHANGE);
            break;
        case TYPE_SWIM_BACK:
            data.Initialize(SMSG_FORCE_SWIM_BACK_SPEED_CHANGE);
            break;
        case TYPE_FLY:
            data.Initialize(SMSG_FORCE_FLIGHT_SPEED_CHANGE);
            break;
        case TYPE_FLY_BACK:
        case TYPE_TURN_RATE:
        case TYPE_WALK:
        case TYPE_PITCH_RATE:
            break;
#endif
    }

    data << GetNewGUID();
#ifdef AE_TBC
    // TODO : Move to own file
    if (speed_type != TYPE_SWIM_BACK)
    {
        data << m_speedChangeCounter++;
        if (speed_type == TYPE_RUN)
            data << uint8_t(1);
    }
    else
    {
        data << uint32_t(0) << uint8_t(0) << uint32_t(Util::getMSTime())
            << GetPosition() << m_position.o << uint32_t(0);
    }
#endif

#ifndef AE_TBC
    BuildMovementPacket(&data);
#endif
    data << float(speed);

    SendMessageToSet(&data, true);
}

void Player::handleFall(MovementInfo const& /*movement_info*/)
{
}

bool Player::isPlayerJumping(MovementInfo const& /*movement_info*/, uint16_t /*opcode*/)
{
    return false;
}

void Player::handleBreathing(MovementInfo const& /*movement_info*/, WorldSession* /*session*/)
{
}

bool Player::isSpellFitByClassAndRace(uint32_t /*spell_id*/)
{
    return false;
}

void Player::sendAuctionCommandResult(Auction* /*auction*/, uint32_t /*action*/, uint32_t /*errorCode*/, uint32_t /*bidError*/)
{
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// Spells
void Player::updateAutoRepeatSpell()
{
    // Get the autorepeat spell
    Spell* autoRepeatSpell = getCurrentSpell(CURRENT_AUTOREPEAT_SPELL);

    // Check is target valid
    // Note for self: remove this check when new Spell::canCast() is ready -Appled
    Unit* target = GetMapMgr()->GetUnit(autoRepeatSpell->m_targets.m_unitTarget);
    if (target == nullptr)
    {
        m_AutoShotAttackTimer = 0;
        interruptSpellWithSpellType(CURRENT_AUTOREPEAT_SPELL);
        return;
    }

    // If player is moving or casting a spell, interrupt wand casting and delay auto shot
    const bool isAutoShot = autoRepeatSpell->GetSpellInfo()->getId() == 75;
    if (m_isMoving || isCastingNonMeleeSpell(true, false, true, isAutoShot))
    {
        if (!isAutoShot)
        {
            interruptSpellWithSpellType(CURRENT_AUTOREPEAT_SPELL);
        }
        m_FirstCastAutoRepeat = true;
        return;
    }

    // Apply delay to wand shooting
    if (m_FirstCastAutoRepeat && m_AutoShotAttackTimer < 500 && !isAutoShot)
    {
        m_AutoShotAttackTimer = 500;
    }
    m_FirstCastAutoRepeat = false;

    if (m_AutoShotAttackTimer == 0)
    {
        // TODO: implement ::CanShootRangedWeapon() into new Spell::canCast()
        // also currently if target gets too far away, your autorepeat spell will get interrupted
        // it's related most likely to ::CanShootRangedWeapon()
        const int32_t canCastAutoRepeatSpell = CanShootRangedWeapon(autoRepeatSpell->GetSpellInfo()->getId(), target, isAutoShot);
        if (canCastAutoRepeatSpell != SPELL_CANCAST_OK)
        {
            if (!isAutoShot)
            {
                interruptSpellWithSpellType(CURRENT_AUTOREPEAT_SPELL);
            }
            return;
        }

        m_AutoShotAttackTimer = getUInt32Value(UNIT_FIELD_RANGEDATTACKTIME);

        // Cast the spell with triggered flag
        Spell* newAutoRepeatSpell = sSpellFactoryMgr.NewSpell(this, autoRepeatSpell->GetSpellInfo(), true, nullptr);
        newAutoRepeatSpell->prepare(&(autoRepeatSpell->m_targets));
    }
}

bool Player::isTransferPending() const
{
    return GetPlayerStatus() == TRANSFER_PENDING;
}

void Player::toggleAfk()
{
    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAG_AFK))
    {
        RemoveFlag(PLAYER_FLAGS, PLAYER_FLAG_AFK);
        if (worldConfig.getKickAFKPlayerTime())
            sEventMgr.RemoveEvents(this, EVENT_PLAYER_SOFT_DISCONNECT);
    }
    else
    {
        SetFlag(PLAYER_FLAGS, PLAYER_FLAG_AFK);

        if (m_bg)
            m_bg->RemovePlayer(this, false);

        if (worldConfig.getKickAFKPlayerTime())
            sEventMgr.AddEvent(this, &Player::SoftDisconnect, EVENT_PLAYER_SOFT_DISCONNECT,
                               worldConfig.getKickAFKPlayerTime(), 1, 0);
    }
}

void Player::toggleDnd()
{
    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAG_DND))
        RemoveFlag(PLAYER_FLAGS, PLAYER_FLAG_DND);
    else
        SetFlag(PLAYER_FLAGS, PLAYER_FLAG_DND);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Messages
void Player::sendReportToGmMessage(std::string playerName, std::string damageLog)
{
    std::string gm_ann(MSG_COLOR_GREEN);

    gm_ann += "|HPlayer:";
    gm_ann += playerName;
    gm_ann += "|h[";
    gm_ann += playerName;
    gm_ann += "]|h: ";
    gm_ann += MSG_COLOR_YELLOW;
    gm_ann += damageLog;

    sWorld.sendMessageToOnlineGms(gm_ann.c_str());
}

//////////////////////////////////////////////////////////////////////////////////////////
// Misc
bool Player::isGMFlagSet()
{
    return HasFlag(PLAYER_FLAGS, PLAYER_FLAG_GM);
}

void Player::sendMovie(uint32_t movieId)
{
#if VERSION_STRING > TBC
    WorldPacket data(SMSG_TRIGGER_MOVIE, 4);
    data << uint32_t(movieId);
    m_session->SendPacket(&data);
#endif
}

uint8 Player::GetPlayerStatus() const { return m_status; }

void Player::setXp(uint32 xp) { write(playerData()->xp, xp); }

uint32 Player::getXp() const { return playerData()->xp; }

void Player::setNextLevelXp(uint32_t xp) { write(playerData()->next_level_xp, xp); }

uint32_t Player::getNextLevelXp() { return playerData()->next_level_xp; }

PlayerSpec& Player::getActiveSpec()
{
#ifdef FT_DUAL_SPEC
    return m_specs[m_talentActiveSpec];
#else
    return m_spec;
#endif
}
