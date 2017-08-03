/*
Copyright (c) 2014-2017 AscEmu Team <http://www.ascemu.org/>
This file is released under the MIT license. See README-MIT for more information.
*/

#pragma once
#include <cstdint>

struct DamageProc
{
    uint32_t m_spellId;
    uint32_t m_damage;
    //uint64 m_caster;          //log is: some reflects x arcane/nature damage to 'attacker' no matter who casted
    uint32_t m_school;
    uint32_t m_flags;
    void* owner;                //mark the owner of this proc to know which one to delete
};

struct SpellCharge
{
    uint32_t spellId;
    uint32_t count;
    uint32_t ProcFlag;
    uint32_t lastproc;
    uint32_t procdiff;
};

enum SpellAttributes
{
    ATTRIBUTES_NULL                                 = 0x00000000,
    ATTRIBUTES_UNK2                                 = 0x00000001,
    ATTRIBUTES_RANGED                               = 0x00000002,   // on next ranged attack
    ATTRIBUTES_ON_NEXT_ATTACK                       = 0x00000004,
    ATTRIBUTES_UNK5                                 = 0x00000008,   // ATTRIBUTES_UNUSED0
    ATTRIBUTES_ABILITY                              = 0x00000010,
    ATTRIBUTES_TRADESPELL                           = 0x00000020,   // Tradeskill recipies
    ATTRIBUTES_PASSIVE                              = 0x00000040,
    ATTRIBUTES_NO_VISUAL_AURA                       = 0x00000080,   // not visible in spellbook or aura bar
    ATTRIBUTES_NO_CAST                              = 0x00000100,   //seems to be afflicts pet
    ATTRIBUTES_TARGET_MAINHAND                      = 0x00000200,   // automatically select item from mainhand
    ATTRIBUTES_ON_NEXT_SWING_2                      = 0x00000400,   //completely the same as ATTRIBUTE_ON_NEXT_ATTACK for class spells. So difference somewhere in mob abilities.
    ATTRIBUTES_UNK13                                = 0x00000800,
    ATTRIBUTES_DAY_ONLY                             = 0x00001000,
    ATTRIBUTES_NIGHT_ONLY                           = 0x00002000,
    ATTRIBUTES_ONLY_INDOORS                         = 0x00004000,
    ATTRIBUTES_ONLY_OUTDOORS                        = 0x00008000,
    ATTRIBUTES_NOT_SHAPESHIFT                       = 0x00010000,
    ATTRIBUTES_REQ_STEALTH                          = 0x00020000,
    ATTRIBUTES_UNK20                                = 0x00040000,   //it's not : must be behind
    ATTRIBUTES_LEVEL_DAMAGE_CALCULATION             = 0x00080000,
    ATTRIBUTES_STOP_ATTACK                          = 0x00100000,   //switch off auto attack on use. Maim,Gouge,Disengage,Polymorph etc
    ATTRIBUTES_CANT_BE_DPB                          = 0x00200000,   //can't be dodged, blocked, parried
    ATTRIBUTES_UNK24                                = 0x00400000,   // related to ranged
    ATTRIBUTES_DEAD_CASTABLE                        = 0x00800000,   //castable while dead
    ATTRIBUTES_MOUNT_CASTABLE                       = 0x01000000,   //castable on mounts
    ATTRIBUTES_TRIGGER_COOLDOWN                     = 0x02000000,   //also requires atributes ex = 32 ?
    ATTRIBUTES_NEGATIVE                             = 0x04000000,   // most negative spells have this attribute
    ATTRIBUTES_CASTABLE_WHILE_SITTING               = 0x08000000,
    ATTRIBUTES_REQ_OOC                              = 0x10000000,   // ATTRIBUTES_REQ_OUT_OF_COMBAT
    ATTRIBUTES_IGNORE_INVULNERABILITY               = 0x20000000,   // debuffs that can't be removed by any spell and spells that can't be resisted in any case
    ATTRIBUTES_HEARTBEAT_RESIST                     = 0x40000000,   // Aura checks for heartbeat resists, possibly resulting to premature aura removal TODO : IMPLEMENT
    ATTRIBUTES_CANT_CANCEL                          = 0x80000000    // seems like aura is not removeable by CMSG_CANCEL_AURA
};

enum SpellAttributesEx
{
    ATTRIBUTESEX_NULL                               = 0x00000000,   // 0
    ATTRIBUTESEX_UNK2                               = 0x00000001,   // 1 pet summonings
    ATTRIBUTESEX_DRAIN_WHOLE_POWER                  = 0x00000002,   // 2 Uses all power / health
    ATTRIBUTESEX_CHANNELED_1                        = 0x00000004,   // 3 Channeled
    ATTRIBUTESEX_UNK5                               = 0x00000008,   // 4
    ATTRIBUTESEX_IGNORE_IN_FRONT                    = 0x00000010,   // 5 ignore verification isInFront() in unit::strike
    ATTRIBUTESEX_NOT_BREAK_STEALTH                  = 0x00000020,   // 6 does not break stealth
    ATTRIBUTESEX_CHANNELED_2                        = 0x00000040,   // 7 Channeled - [POSSIBLY: dynamite, grenades from engineering etc..]
    ATTRIBUTESEX_NEGATIVE                           = 0x00000080,   // 8
    ATTRIBUTESEX_REQ_OOC_TARGET                     = 0x00000100,   // 9 Spell req target should not be in combat
    ATTRIBUTESEX_REQ_FACING_TARGET                  = 0x00000200,   // 10 not 100% sure
    ATTRIBUTESEX_NO_INITIAL_AGGRO                   = 0x00000400,   // 11 guessed
    ATTRIBUTESEX_UNK13                              = 0x00000800,   // 12
    ATTRIBUTESEX_UNK14                              = 0x00001000,   // 13 related to pickpocket
    ATTRIBUTESEX_UNK15                              = 0x00002000,   // 14 related to remote control
    ATTRIBUTESEX_CHANNEL_FACE_TARGET                = 0x00004000,   // 15 Client keeps facting target while channeling
    ATTRIBUTESEX_DISPEL_AURAS_ON_IMMUNITY           = 0x00008000,   // 16 remove auras on immunity - something like "grant immunity"
    ATTRIBUTESEX_UNAFFECTED_BY_SCHOOL_IMMUNE        = 0x00010000,   // 17 unaffected by school immunity - something like "grant immunity" too
    ATTRIBUTESEX_REMAIN_OOC                         = 0x00020000,   // 18
    ATTRIBUTESEX_UNK20                              = 0x00040000,   // 19
    ATTRIBUTESEX_CANT_TARGET_SELF                   = 0x00080000,   // 20
    ATTRIBUTESEX_REQ_COMBO_POINTS1                  = 0x00100000,   // 21 related to "Finishing move" and "Instantly overpowers"
    ATTRIBUTESEX_UNK23                              = 0x00200000,   // 22
    ATTRIBUTESEX_REQ_COMBO_POINTS2                  = 0x00400000,   // 23 only related to "Finishing move"
    ATTRIBUTESEX_UNK25                              = 0x00800000,   // 24 related to spells like "ClearAllBuffs"
    ATTRIBUTESEX_UNK26                              = 0x01000000,   // 25 req fishing pole? FISHING SPELLS
    ATTRIBUTESEX_UNK27                              = 0x02000000,   // 26 related to "Detect" spell
    ATTRIBUTESEX_UNK28                              = 0x04000000,   // 27
    ATTRIBUTESEX_UNK29                              = 0x08000000,   // 28
    ATTRIBUTESEX_UNK30                              = 0x10000000,   // 29
    ATTRIBUTESEX_UNK31                              = 0x20000000,   // 30
    ATTRIBUTESEX_UNK32                              = 0x40000000,   // 31 Overpower
    ATTRIBUTESEX_UNK33                              = 0x80000000    // 32
};

enum SpellAttributesExB
{
    ATTRIBUTESEXB_NULL                              = 0x00000000,   // 0
    ATTRIBUTESEXB_CAN_CAST_ON_DEAD_UNIT             = 0x00000001,   // 1
    ATTRIBUTESEXB_UNK3                              = 0x00000002,   // 2 Can be used while stealthed
    ATTRIBUTESEXB_UNK4                              = 0x00000004,   // 3 request pet maybe
    ATTRIBUTESEXB_UNK5                              = 0x00000008,   // 4 something todo with temp enchanted items
    ATTRIBUTESEXB_PARTY_EFFECTING_AURA              = 0x00000010,   // 5 Party affecting aura's
    ATTRIBUTESEXB_AUTO_REPEAT                       = 0x00000020,   // 6 Auto-attack and auto-shoot for example
    ATTRIBUTESEXB_CANT_TARGET_TAPPED                = 0x00000040,   // 7 Target must be tapped by caster
    ATTRIBUTESEXB_UNK9                              = 0x00000080,   // 8
    ATTRIBUTESEXB_UNUSED1                           = 0x00000100,   // 9 not set in 3.0.3
    ATTRIBUTESEXB_UNK11                             = 0x00000200,   // 10 used by 2 spells, 30421 | Nether Portal - Perseverence and  30466 | Nether Portal - Perseverence
    ATTRIBUTESEXB_TAME_X                            = 0x00000400,   // 11 tame [creature]
    ATTRIBUTESEXB_FUNNEL                            = 0x00000800,   // 12 only funnel spells
    ATTRIBUTESEXB_UNK14                             = 0x00001000,   // 13 swipe / Cleave spells
    ATTRIBUTESEXB_ENCHANT_OWN_ONLY                  = 0x00002000,   // 14 no trade window targets, BoE items get soulbound to you
    ATTRIBUTESEXB_SPELL_PLAYER_EVENT                = 0x00004000,   // 15 Player event's like logging in, finishing quests, triggering cinematic, being adored, Heartbroken etc
    ATTRIBUTESEXB_UNUSED3                           = 0x00008000,   // 16 not set in 3.0.3
    ATTRIBUTESEXB_CONTROL_UNIT                      = 0x00010000,   // 17 PvP Controller, RC, Creature taming, Taming Lesson
    ATTRIBUTESEXB_DONT_RESET_AUTO_ACTIONS           = 0x00020000,   // 18 spell with this attribute does not reset auto shoot or melee auto attack timers
    ATTRIBUTESEXB_REQ_DEAD_PET                      = 0x00040000,   // 19
    ATTRIBUTESEXB_NOT_NEED_SHAPESHIFT               = 0x00080000,   // 20 does not necessarily need shapeshift
    ATTRIBUTESEXB_REQ_BEHIND_TARGET                 = 0x00100000,   // 21
    ATTRIBUTESEXB_UNK23                             = 0x00200000,   // 22
    ATTRIBUTESEXB_UNK24                             = 0x00400000,   // 23
    ATTRIBUTESEXB_UNK25                             = 0x00800000,   // 24
    ATTRIBUTESEXB_UNK26                             = 0x01000000,   // 25
    ATTRIBUTESEXB_UNK27                             = 0x02000000,   // 26
    ATTRIBUTESEXB_UNK28                             = 0x04000000,   // 27 unaffected by school immunity
    ATTRIBUTESEXB_UNK29                             = 0x08000000,   // 28 fishing spells and enchanting weapons
    ATTRIBUTESEXB_IGNORE_ITEM_CHECKS                = 0x10000000,   // 29 ignores item requirements on spell cast
    ATTRIBUTESEXB_CANT_CRIT                         = 0x20000000,   // 30 spell can't crit
    ATTRIBUTESEXB_UNK32                             = 0x40000000,   // 31
    ATTRIBUTESEXB_UNK33                             = 0x80000000    // 32
};

enum SpellAttributesExC
{
    ATTRIBUTESEXC_NULL                              = 0x00000000,
    ATTRIBUTESEXC_UNK2                              = 0x00000001,
    ATTRIBUTESEXC_UNK3                              = 0x00000002,
    ATTRIBUTESEXC_UNK4                              = 0x00000004,
    ATTRIBUTESEXC_UNK5                              = 0x00000008,
    ATTRIBUTESEXC_UNK6                              = 0x00000010,   // ignor resurrection
    ATTRIBUTESEXC_UNK7                              = 0x00000020,
    ATTRIBUTESEXC_UNK8                              = 0x00000040,
    ATTRIBUTESEXC_UNK9                              = 0x00000080,
    ATTRIBUTESEXC_ONLY_PLAYER_TARGETS               = 0x00000100,   // spell can only be casted on player targets
    ATTRIBUTESEXC_UNK11                             = 0x00000200,
    ATTRIBUTESEXC_UNK12                             = 0x00000400,
    ATTRIBUTESEXC_BG_ONLY                           = 0x00000800,
    ATTRIBUTESEXC_TARGET_ONLY_GHOSTS                = 0x00001000,
    ATTRIBUTESEXC_UNK15                             = 0x00002000,
    ATTRIBUTESEXC_UNK16                             = 0x00004000,
    ATTRIBUTESEXC_PLAYER_RANGED_SPELLS              = 0x00008000,
    ATTRIBUTESEXC_UNK18                             = 0x00010000,
    ATTRIBUTESEXC_UNK19                             = 0x00020000,
    ATTRIBUTESEXC_UNK20                             = 0x00040000,
    ATTRIBUTESEXC_UNK21                             = 0x00080000,   // e.g. Totemic mastery
    ATTRIBUTESEXC_CAN_PERSIST_AND_CASTED_WHILE_DEAD = 0x00100000,
    ATTRIBUTESEXC_UNK23                             = 0x00200000,
    ATTRIBUTESEXC_PLAYER_REQUIRED_WAND              = 0x00400000,   // required wand
    ATTRIBUTESEXC_UNK25                             = 0x00800000,
    ATTRIBUTESEXC_TYPE_OFFHAND                      = 0x01000000,   // required offhand
    ATTRIBUTESEXC_NO_HEALING_BONUS                  = 0x02000000,
    ATTRIBUTESEXC_CAN_PROC_ON_TRIGGERED             = 0x04000000,
    ATTRIBUTESEXC_DRAIN_SOUL                        = 0x08000000,   // just drain soul has this flag
    ATTRIBUTESEXC_UNK30                             = 0x10000000,
    ATTRIBUTESEXC_NO_DONE_BONUS                     = 0x20000000,   ///\todo used for checking spellpower/damage mods
    ATTRIBUTESEXC_NO_DISPLAY_RANGE                  = 0x40000000,   // tooltip dont show range
    ATTRIBUTESEXC_UNK33                             = 0x80000000
};

enum SpellAttributesExD
{
    ATTRIBUTESEXD_NULL                              = 0x00000000,
    ATTRIBUTESEXD_UNK1                              = 0x00000001,
    ATTRIBUTESEXD_PROCCHANCE_COMBOBASED             = 0x00000002,
    ATTRIBUTESEXD_UNK2                              = 0x00000004,
    ATTRIBUTESEXD_UNK3                              = 0x00000008,
    ATTRIBUTESEXD_UNK4                              = 0x00000010,
    ATTRIBUTESEXD_UNK5                              = 0x00000020,
    ATTRIBUTESEXD_NOT_STEALABLE                     = 0x00000040,
    ATTRIBUTESEXD_TRIGGERED                         = 0x00000080,   // spells forced to be triggered
    ATTRIBUTESEXD_UNK6                              = 0x00000100,
    ATTRIBUTESEXD_TRIGGER_ACTIVATE                  = 0x00000200,   // trigger activate (Deep Freeze...)
    ATTRIBUTESEXD_UNK7                              = 0x00000400,   // only rogue's Shiv has this attribute in 3.3.5a
    ATTRIBUTESEXD_UNK8                              = 0x00000800,
    ATTRIBUTESEXD_UNK9                              = 0x00001000,
    ATTRIBUTESEXD_UNK10                             = 0x00002000,
    ATTRIBUTESEXD_NOT_BREAK_AURAS                   = 0x00004000,   // not breake auras by damage from this spell
    ATTRIBUTESEXD_UNK11                             = 0x00008000,
    ATTRIBUTESEXD_NOT_IN_ARENA                      = 0x00010000,   // can not be used in arenas
    ATTRIBUTESEXD_USABLE_IN_ARENA                   = 0x00020000,   // can be used in arenas
    ATTRIBUTESEXD_UNK13                             = 0x00040000,
    ATTRIBUTESEXD_UNK14                             = 0x00080000,
    ATTRIBUTESEXD_UNK15                             = 0x00100000,
    ATTRIBUTESEXD_UNK16                             = 0x00200000,   // pala aura, dk presence, dudu form, warrior stance, shadowform, hunter track
    ATTRIBUTESEXD_UNK17                             = 0x00400000,
    ATTRIBUTESEXD_UNK18                             = 0x00800000,
    ATTRIBUTESEXD_UNK19                             = 0x01000000,
    ATTRIBUTESEXD_NOT_USED                          = 0x02000000,
    ATTRIBUTESEXD_ONLY_IN_OUTLANDS                  = 0x04000000,   // can be used only in outland
    ATTRIBUTESEXD_UNK20                             = 0x08000000,
    ATTRIBUTESEXD_UNK21                             = 0x10000000,
    ATTRIBUTESEXD_UNK22                             = 0x20000000,
    ATTRIBUTESEXD_UNK23                             = 0x40000000,
    ATTRIBUTESEXD_UNK24                             = 0x80000000
};

enum SpellAttributesExE
{
    ATTRIBUTESEXE_NULL                              = 0x00000000,
    ATTRIBUTESEXE_UNK2                              = 0x00000001,
    ATTRIBUTESEXE_REAGENT_REMOVAL                   = 0x00000002,
    ATTRIBUTESEXE_UNK4                              = 0x00000004,
    ATTRIBUTESEXE_USABLE_WHILE_STUNNED              = 0x00000008,   // usable while stunned
    ATTRIBUTESEXE_UNK6                              = 0x00000010,
    ATTRIBUTESEXE_SINGLE_TARGET_AURA                = 0x00000020,   // only one target can be applied
    ATTRIBUTESEXE_UNK8                              = 0x00000040,
    ATTRIBUTESEXE_UNK9                              = 0x00000080,
    ATTRIBUTESEXE_UNK10                             = 0x00000100,
    ATTRIBUTESEXE_UNK11                             = 0x00000200,   // periodic aura apply
    ATTRIBUTESEXE_HIDE_DURATION                     = 0x00000400,   // no duration for client
    ATTRIBUTESEXE_UNK13                             = 0x00000800,
    ATTRIBUTESEXE_UNK14                             = 0x00001000,
    ATTRIBUTESEXE_HASTE_AFFECTS_DURATION            = 0x00002000,
    ATTRIBUTESEXE_UNK16                             = 0x00004000,
    ATTRIBUTESEXE_UNK17                             = 0x00008000,
    ATTRIBUTESEXE_ITEM_CLASS_CHECK                  = 0x00010000,   ///\todo this allows spells with EquippedItemClass to affect spells from other items if the required item is equipped
    ATTRIBUTESEXE_USABLE_WHILE_FEARED               = 0x00020000,   // usable while feared
    ATTRIBUTESEXE_USABLE_WHILE_CONFUSED             = 0x00040000,   // usable while confused
    ATTRIBUTESEXE_UNK21                             = 0x00080000,
    ATTRIBUTESEXE_UNK22                             = 0x00100000,
    ATTRIBUTESEXE_UNK23                             = 0x00200000,
    ATTRIBUTESEXE_UNK24                             = 0x00400000,
    ATTRIBUTESEXE_UNK25                             = 0x00800000,
    ATTRIBUTESEXE_UNK26                             = 0x01000000,
    ATTRIBUTESEXE_UNK27                             = 0x02000000,
    ATTRIBUTESEXE_UNK28                             = 0x04000000,
    ATTRIBUTESEXE_UNK29                             = 0x08000000,
    ATTRIBUTESEXE_UNK30                             = 0x10000000,
    ATTRIBUTESEXE_UNK31                             = 0x20000000,
    ATTRIBUTESEXE_UNK32                             = 0x40000000,
    ATTRIBUTESEXE_UNK33                             = 0x80000000
};

enum SpellAttributesExF
{
    ATTRIBUTESEXF_NULL                              = 0x00000000,
    ATTRIBUTESEXF_UNK2                              = 0x00000001,   // cooldown in tooltyp (not displayed)
    ATTRIBUTESEXF_UNUSED1                           = 0x00000002,   // only arena
    ATTRIBUTESEXF_UNK4                              = 0x00000004,
    ATTRIBUTESEXF_UNK5                              = 0x00000008,
    ATTRIBUTESEXF_UNK6                              = 0x00000010,
    ATTRIBUTESEXF_UNK7                              = 0x00000020,
    ATTRIBUTESEXF_UNK8                              = 0x00000040,
    ATTRIBUTESEXF_UNK9                              = 0x00000080,
    ATTRIBUTESEXF_UNK10                             = 0x00000100,
    ATTRIBUTESEXF_UNK11                             = 0x00000200,
    ATTRIBUTESEXF_UNK12                             = 0x00000400,
    ATTRIBUTESEXF_NOT_IN_RAID_OR_HEROIC_DUNGEONS    = 0x00000800,   // Not in raids or heroic dungeons
    ATTRIBUTESEXF_UNUSED4                           = 0x00001000,   // castable on vehicle
    ATTRIBUTESEXF_CAN_TARGET_INVISIBLE              = 0x00002000,
    ATTRIBUTESEXF_UNUSED5                           = 0x00004000,
    ATTRIBUTESEXF_UNUSED6                           = 0x00008000,   // 54368, 67892
    ATTRIBUTESEXF_UNUSED7                           = 0x00010000,
    ATTRIBUTESEXF_UNK19                             = 0x00020000,   // Mountspells?
    ATTRIBUTESEXF_CAST_BY_CHARMER                   = 0x00040000,
    ATTRIBUTESEXF_UNK21                             = 0x00080000,
    ATTRIBUTESEXF_UNK22                             = 0x00100000,
    ATTRIBUTESEXF_UNK23                             = 0x00200000,
    ATTRIBUTESEXF_UNK24                             = 0x00400000,
    ATTRIBUTESEXF_UNK25                             = 0x00800000,
    ATTRIBUTESEXF_UNK26                             = 0x01000000,
    ATTRIBUTESEXF_UNK27                             = 0x02000000,
    ATTRIBUTESEXF_UNK28                             = 0x04000000,
    ATTRIBUTESEXF_UNK29                             = 0x08000000,
    ATTRIBUTESEXF_UNK30                             = 0x10000000,
    ATTRIBUTESEXF_UNK31                             = 0x20000000,
    ATTRIBUTESEXF_UNK32                             = 0x40000000,
    ATTRIBUTESEXF_UNK33                             = 0x80000000
};

enum SpellAttributesExG
{
    ATTRIBUTESEXG_NULL                              = 0x00000000,
    ATTRIBUTESEXG_UNK1                              = 0x00000001,
    ATTRIBUTESEXG_UNK2                              = 0x00000002,
    ATTRIBUTESEXG_IGNORE_CASTER_AURAS               = 0x00000004,
    ATTRIBUTESEXG_IS_CHEAT_SPELL                    = 0x00000008,
    ATTRIBUTESEXG_UNK5                              = 0x00000010,
    ATTRIBUTESEXG_UNK6                              = 0x00000020,   // shaman player totem summon?
    ATTRIBUTESEXG_UNK7                              = 0x00000040,
    ATTRIBUTESEXG_UNK8                              = 0x00000080,
    ATTRIBUTESEXG_UNK9                              = 0x00000100,
    ATTRIBUTESEXG_UNK10                             = 0x00000200,
    ATTRIBUTESEXG_UNK11                             = 0x00000400,
    ATTRIBUTESEXG_INTERRUPT_NPC                     = 0x00000800,   // non player character casts interruption
    ATTRIBUTESEXG_UNK13                             = 0x00001000,
    ATTRIBUTESEXG_UNK14                             = 0x00002000,
    ATTRIBUTESEXG_UNK15                             = 0x00004000,
    ATTRIBUTESEXG_UNK16                             = 0x00008000,
    ATTRIBUTESEXG_UNK17                             = 0x00010000,
    ATTRIBUTESEXG_UNK18                             = 0x00020000,
    ATTRIBUTESEXG_UNK19                             = 0x00040000,
    ATTRIBUTESEXG_UNK20                             = 0x00080000,
    ATTRIBUTESEXG_UNK21                             = 0x00100000,
    ATTRIBUTESEXG_UNK22                             = 0x00200000,
    ATTRIBUTESEXG_IGNORE_COLD_WEATHER_FLYING        = 0x00400000,
    ATTRIBUTESEXG_UNK24                             = 0x00800000,
    ATTRIBUTESEXG_UNK25                             = 0x01000000,
    ATTRIBUTESEXG_UNK26                             = 0x02000000,
    ATTRIBUTESEXG_UNK27                             = 0x04000000,
    ATTRIBUTESEXG_UNK28                             = 0x08000000,
    ATTRIBUTESEXG_UNK29                             = 0x10000000,
    ATTRIBUTESEXG_UNK30                             = 0x20000000,
    ATTRIBUTESEXG_UNK31                             = 0x40000000,
    ATTRIBUTESEXG_UNK32                             = 0x80000000
};

#if VERSION_STRING == Cata
enum SpellAttributesExH
{
    ATTRIBUTESEXH_UNK0                              = 0x00000001,   // 0
    ATTRIBUTESEXH_UNK1                              = 0x00000002,   // 1 
    ATTRIBUTESEXH_UNK2                              = 0x00000004,   // 2 
    ATTRIBUTESEXH_UNK3                              = 0x00000008,   // 3
    ATTRIBUTESEXH_UNK4                              = 0x00000010,   // 4
    ATTRIBUTESEXH_UNK5                              = 0x00000020,   // 5
    ATTRIBUTESEXH_UNK6                              = 0x00000040,   // 6 
    ATTRIBUTESEXH_UNK7                              = 0x00000080,   // 7
    ATTRIBUTESEXH_UNK8                              = 0x00000100,   // 8 
    ATTRIBUTESEXH_UNK9                              = 0x00000200,   // 9 
    ATTRIBUTESEXH_UNK10                             = 0x00000400,   // 10 
    ATTRIBUTESEXH_UNK11                             = 0x00000800,   // 11 
    ATTRIBUTESEXH_UNK12                             = 0x00001000,   // 12
    ATTRIBUTESEXH_UNK13                             = 0x00002000,   // 13
    ATTRIBUTESEXH_UNK14                             = 0x00004000,   // 14 
    ATTRIBUTESEXH_UNK15                             = 0x00008000,   // 15 
    ATTRIBUTESEXH_UNK16                             = 0x00010000,   // 16
    ATTRIBUTESEXH_UNK17                             = 0x00020000,   // 17
    ATTRIBUTESEXH_UNK18                             = 0x00040000,   // 18 
    ATTRIBUTESEXH_UNK19                             = 0x00080000,   // 19 
    ATTRIBUTESEXH_UNK20                             = 0x00100000,   // 20
    ATTRIBUTESEXH_UNK21                             = 0x00200000,   // 21 
    ATTRIBUTESEXH_UNK22                             = 0x00400000,   // 22 
    ATTRIBUTESEXH_UNK23                             = 0x00800000,   // 23 
    ATTRIBUTESEXH_UNK24                             = 0x01000000,   // 24 
    ATTRIBUTESEXH_UNK25                             = 0x02000000,   // 25 
    ATTRIBUTESEXH_UNK26                             = 0x04000000,   // 26 
    ATTRIBUTESEXH_UNK27                             = 0x08000000,   // 27
    ATTRIBUTESEXH_UNK28                             = 0x10000000,   // 28
    ATTRIBUTESEXH_UNK29                             = 0x20000000,   // 29
    ATTRIBUTESEXH_UNK30                             = 0x40000000,   // 30
    ATTRIBUTESEXH_UNK31                             = 0x80000000    // 31
};

enum SpellAttributesExI
{
    ATTRIBUTESEXI_UNK0                              = 0x00000001,   // 0
    ATTRIBUTESEXI_UNK1                              = 0x00000002,   // 1
    ATTRIBUTESEXI_UNK2                              = 0x00000004,   // 2
    ATTRIBUTESEXI_UNK3                              = 0x00000008,   // 3
    ATTRIBUTESEXI_UNK4                              = 0x00000010,   // 4
    ATTRIBUTESEXI_UNK5                              = 0x00000020,   // 5
    ATTRIBUTESEXI_UNK6                              = 0x00000040,   // 6
    ATTRIBUTESEXI_UNK7                              = 0x00000080,   // 7
    ATTRIBUTESEXI_UNK8                              = 0x00000100,   // 8
    ATTRIBUTESEXI_UNK9                              = 0x00000200,   // 9
    ATTRIBUTESEXI_UNK10                             = 0x00000400,   // 10
    ATTRIBUTESEXI_UNK11                             = 0x00000800,   // 11
    ATTRIBUTESEXI_UNK12                             = 0x00001000,   // 12
    ATTRIBUTESEXI_UNK13                             = 0x00002000,   // 13
    ATTRIBUTESEXI_UNK14                             = 0x00004000,   // 14
    ATTRIBUTESEXI_UNK15                             = 0x00008000,   // 15
    ATTRIBUTESEXI_UNK16                             = 0x00010000,   // 16
    ATTRIBUTESEXI_UNK17                             = 0x00020000,   // 17
    ATTRIBUTESEXI_UNK18                             = 0x00040000,   // 18
    ATTRIBUTESEXI_UNK19                             = 0x00080000,   // 19
    ATTRIBUTESEXI_UNK20                             = 0x00100000,   // 20
    ATTRIBUTESEXI_UNK21                             = 0x00200000,   // 21
    ATTRIBUTESEXI_UNK22                             = 0x00400000,   // 22
    ATTRIBUTESEXI_UNK23                             = 0x00800000,   // 23
    ATTRIBUTESEXI_UNK24                             = 0x01000000,   // 24
    ATTRIBUTESEXI_UNK25                             = 0x02000000,   // 25
    ATTRIBUTESEXI_UNK26                             = 0x04000000,   // 26
    ATTRIBUTESEXI_UNK27                             = 0x08000000,   // 27
    ATTRIBUTESEXI_UNK28                             = 0x10000000,   // 28
    ATTRIBUTESEXI_UNK29                             = 0x20000000,   // 29
    ATTRIBUTESEXI_UNK30                             = 0x40000000,   // 30
    ATTRIBUTESEXI_UNK31                             = 0x80000000    // 31
};

enum SpellAttributesExJ
{
    ATTRIBUTESEXJ_UNK0                              = 0x00000001,   // 0
    ATTRIBUTESEXJ_UNK1                              = 0x00000002,   // 1
    ATTRIBUTESEXJ_UNK2                              = 0x00000004,   // 2
    ATTRIBUTESEXJ_UNK3                              = 0x00000008,   // 3
    ATTRIBUTESEXJ_UNK4                              = 0x00000010,   // 4
    ATTRIBUTESEXJ_UNK5                              = 0x00000020,   // 5
    ATTRIBUTESEXJ_UNK6                              = 0x00000040,   // 6
    ATTRIBUTESEXJ_UNK7                              = 0x00000080,   // 7
    ATTRIBUTESEXJ_UNK8                              = 0x00000100,   // 8
    ATTRIBUTESEXJ_UNK9                              = 0x00000200,   // 9
    ATTRIBUTESEXJ_UNK10                             = 0x00000400,   // 10
    ATTRIBUTESEXJ_UNK11                             = 0x00000800,   // 11
    ATTRIBUTESEXJ_UNK12                             = 0x00001000,   // 12
    ATTRIBUTESEXJ_UNK13                             = 0x00002000,   // 13
    ATTRIBUTESEXJ_UNK14                             = 0x00004000,   // 14
    ATTRIBUTESEXJ_UNK15                             = 0x00008000,   // 15
    ATTRIBUTESEXJ_UNK16                             = 0x00010000,   // 16
    ATTRIBUTESEXJ_UNK17                             = 0x00020000,   // 17
    ATTRIBUTESEXJ_UNK18                             = 0x00040000,   // 18
    ATTRIBUTESEXJ_UNK19                             = 0x00080000,   // 19
    ATTRIBUTESEXJ_UNK20                             = 0x00100000,   // 20
    ATTRIBUTESEXJ_UNK21                             = 0x00200000,   // 21
    ATTRIBUTESEXJ_UNK22                             = 0x00400000,   // 22
    ATTRIBUTESEXJ_UNK23                             = 0x00800000,   // 23
    ATTRIBUTESEXJ_UNK24                             = 0x01000000,   // 24
    ATTRIBUTESEXJ_UNK25                             = 0x02000000,   // 25
    ATTRIBUTESEXJ_UNK26                             = 0x04000000,   // 26
    ATTRIBUTESEXJ_UNK27                             = 0x08000000,   // 27
    ATTRIBUTESEXJ_UNK28                             = 0x10000000,   // 28
    ATTRIBUTESEXJ_UNK29                             = 0x20000000,   // 29
    ATTRIBUTESEXJ_UNK30                             = 0x40000000,   // 30
    ATTRIBUTESEXJ_UNK31                             = 0x80000000    // 31
};
#endif

enum AuraModTypes
{
    SPELL_AURA_NONE = 0,                                                // None
    SPELL_AURA_BIND_SIGHT = 1,                                          // Bind Sight
    SPELL_AURA_MOD_POSSESS = 2,                                         // Mod Possess
    SPELL_AURA_PERIODIC_DAMAGE = 3,                                     // Periodic Damage
    SPELL_AURA_DUMMY = 4,                                               // Script Aura
    SPELL_AURA_MOD_CONFUSE = 5,                                         // Mod Confuse
    SPELL_AURA_MOD_CHARM = 6,                                           // Mod Charm
    SPELL_AURA_MOD_FEAR = 7,                                            // Mod Fear
    SPELL_AURA_PERIODIC_HEAL = 8,                                       // Periodic Heal
    SPELL_AURA_MOD_ATTACKSPEED = 9,                                     // Mod Attack Speed
    SPELL_AURA_MOD_THREAT = 10,                                         // Mod Threat
    SPELL_AURA_MOD_TAUNT = 11,                                          // Taunt
    SPELL_AURA_MOD_STUN = 12,                                           // Stun
    SPELL_AURA_MOD_DAMAGE_DONE = 13,                                    // Mod Damage Done
    SPELL_AURA_MOD_DAMAGE_TAKEN = 14,                                   // Mod Damage Taken
    SPELL_AURA_DAMAGE_SHIELD = 15,                                      // Damage Shield
    SPELL_AURA_MOD_STEALTH = 16,                                        // Mod Stealth
    SPELL_AURA_MOD_DETECT = 17,                                         // Mod Detect
    SPELL_AURA_MOD_INVISIBILITY = 18,                                   // Mod Invisibility
    SPELL_AURA_MOD_INVISIBILITY_DETECTION = 19,                         // Mod Invisibility Detection
    SPELL_AURA_MOD_TOTAL_HEALTH_REGEN_PCT = 20,
    SPELL_AURA_MOD_TOTAL_MANA_REGEN_PCT = 21,
    SPELL_AURA_MOD_RESISTANCE = 22,                                     // Mod Resistance
    SPELL_AURA_PERIODIC_TRIGGER_SPELL = 23,                             // Periodic Trigger
    SPELL_AURA_PERIODIC_ENERGIZE = 24,                                  // Periodic Energize
    SPELL_AURA_MOD_PACIFY = 25,                                         // Pacify
    SPELL_AURA_MOD_ROOT = 26,                                           // Root
    SPELL_AURA_MOD_SILENCE = 27,                                        // Silence
    SPELL_AURA_REFLECT_SPELLS = 28,                                     // Reflect Spells %
    SPELL_AURA_MOD_STAT = 29,                                           // Mod Stat
    SPELL_AURA_MOD_SKILL = 30,                                          // Mod Skill
    SPELL_AURA_MOD_INCREASE_SPEED = 31,                                 // Mod Speed
    SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED = 32,                         // Mod Speed Mounted
    SPELL_AURA_MOD_DECREASE_SPEED = 33,                                 // Mod Speed Slow
    SPELL_AURA_MOD_INCREASE_HEALTH = 34,                                // Mod Increase Health
    SPELL_AURA_MOD_INCREASE_ENERGY = 35,                                // Mod Increase Energy
    SPELL_AURA_MOD_SHAPESHIFT = 36,                                     // Shapeshift
    SPELL_AURA_EFFECT_IMMUNITY = 37,                                    // Immune Effect
    SPELL_AURA_STATE_IMMUNITY = 38,                                     // Immune State
    SPELL_AURA_SCHOOL_IMMUNITY = 39,                                    // Immune School
    SPELL_AURA_DAMAGE_IMMUNITY = 40,                                    // Immune Damage
    SPELL_AURA_DISPEL_IMMUNITY = 41,                                    // Immune Dispel Type
    SPELL_AURA_PROC_TRIGGER_SPELL = 42,                                 // Proc Trigger Spell
    SPELL_AURA_PROC_TRIGGER_DAMAGE = 43,                                // Proc Trigger Damage
    SPELL_AURA_TRACK_CREATURES = 44,                                    // Track Creatures
    SPELL_AURA_TRACK_RESOURCES = 45,                                    // Track Resources
    SPELL_AURA_MOD_PARRY_SKILL = 46,                                    // Mod Parry Skill
    SPELL_AURA_MOD_PARRY_PERCENT = 47,                                  // Mod Parry Percent
    SPELL_AURA_MOD_DODGE_SKILL = 48,                                    // Mod Dodge Skill
    SPELL_AURA_MOD_DODGE_PERCENT = 49,                                  // Mod Dodge Percent
    SPELL_AURA_MOD_BLOCK_SKILL = 50,                                    // Mod Block Skill
    SPELL_AURA_MOD_BLOCK_PERCENT = 51,                                  // Mod Block Percent
    SPELL_AURA_MOD_CRIT_PERCENT = 52,                                   // Mod Crit Percent
    SPELL_AURA_PERIODIC_LEECH = 53,                                     // Periodic Leech
    SPELL_AURA_MOD_HIT_CHANCE = 54,                                     // Mod Hit Chance
    SPELL_AURA_MOD_SPELL_HIT_CHANCE = 55,                               // Mod Spell Hit Chance
    SPELL_AURA_TRANSFORM = 56,                                          // Transform
    SPELL_AURA_MOD_SPELL_CRIT_CHANCE = 57,                              // Mod Spell Crit Chance
    SPELL_AURA_MOD_INCREASE_SWIM_SPEED = 58,                            // Mod Speed Swim
    SPELL_AURA_MOD_DAMAGE_DONE_CREATURE = 59,                           // Mod Creature Dmg Done
    SPELL_AURA_MOD_PACIFY_SILENCE = 60,                                 // Pacify & Silence
    SPELL_AURA_MOD_SCALE = 61,                                          // Mod Scale
    SPELL_AURA_PERIODIC_HEALTH_FUNNEL = 62,                             // Periodic Health Funnel
    SPELL_AURA_PERIODIC_MANA_FUNNEL = 63,                               // Periodic Mana Funnel
    SPELL_AURA_PERIODIC_MANA_LEECH = 64,                                // Periodic Mana Leech
    SPELL_AURA_MOD_CASTING_SPEED = 65,                                  // Haste - Spells
    SPELL_AURA_FEIGN_DEATH = 66,                                        // Feign Death
    SPELL_AURA_MOD_DISARM = 67,                                         // Disarm
    SPELL_AURA_MOD_STALKED = 68,                                        // Mod Stalked
    SPELL_AURA_SCHOOL_ABSORB = 69,                                      // School Absorb
    SPELL_AURA_EXTRA_ATTACKS = 70,                                      // Extra Attacks
    SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL = 71,                       // Mod School Spell Crit Chance
    SPELL_AURA_MOD_POWER_COST = 72,                                     // Mod Power Cost
    SPELL_AURA_MOD_POWER_COST_SCHOOL = 73,                              // Mod School Power Cost
    SPELL_AURA_REFLECT_SPELLS_SCHOOL = 74,                              // Reflect School Spells %
    SPELL_AURA_MOD_LANGUAGE = 75,                                       // Mod Language
    SPELL_AURA_FAR_SIGHT = 76,                                          // Far Sight
    SPELL_AURA_MECHANIC_IMMUNITY = 77,                                  // Immune Mechanic
    SPELL_AURA_MOUNTED = 78,                                            // Mounted
    SPELL_AURA_MOD_DAMAGE_PERCENT_DONE = 79,                            // Mod Dmg %
    SPELL_AURA_MOD_PERCENT_STAT = 80,                                   // Mod Stat %
    SPELL_AURA_SPLIT_DAMAGE = 81,                                       // Split Damage
    SPELL_AURA_WATER_BREATHING = 82,                                    // Water Breathing
    SPELL_AURA_MOD_BASE_RESISTANCE = 83,                                // Mod Base Resistance
    SPELL_AURA_MOD_REGEN = 84,                                          // Mod Health Regen
    SPELL_AURA_MOD_POWER_REGEN = 85,                                    // Mod Power Regen
    SPELL_AURA_CHANNEL_DEATH_ITEM = 86,                                 // Create Death Item
    SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN = 87,                           // Mod Dmg % Taken
    SPELL_AURA_MOD_PERCENT_REGEN = 88,                                  // Mod Health Regen Percent
    SPELL_AURA_PERIODIC_DAMAGE_PERCENT = 89,                            // Periodic Damage Percent
    SPELL_AURA_MOD_RESIST_CHANCE = 90,                                  // Mod Resist Chance
    SPELL_AURA_MOD_DETECT_RANGE = 91,                                   // Mod Detect Range
    SPELL_AURA_PREVENTS_FLEEING = 92,                                   // Prevent Fleeing
    SPELL_AURA_MOD_UNATTACKABLE = 93,                                   // Mod Unintractable
    SPELL_AURA_INTERRUPT_REGEN = 94,                                    // Interrupt Regen
    SPELL_AURA_GHOST = 95,                                              // Ghost
    SPELL_AURA_SPELL_MAGNET = 96,                                       // Spell Magnet
    SPELL_AURA_MANA_SHIELD = 97,                                        // Mana Shield
    SPELL_AURA_MOD_SKILL_TALENT = 98,                                   // Mod Skill Talent
    SPELL_AURA_MOD_ATTACK_POWER = 99,                                   // Mod Attack Power
    SPELL_AURA_AURAS_VISIBLE = 100,                                     // Auras Visible
    SPELL_AURA_MOD_RESISTANCE_PCT = 101,                                // Mod Resistance %
    SPELL_AURA_MOD_CREATURE_ATTACK_POWER = 102,                         // Mod Creature Attack Power
    SPELL_AURA_MOD_TOTAL_THREAT = 103,                                  // Mod Total Threat (Fade)
    SPELL_AURA_WATER_WALK = 104,                                        // Water Walk
    SPELL_AURA_FEATHER_FALL = 105,                                      // Feather Fall
    SPELL_AURA_HOVER = 106,                                             // Hover
    SPELL_AURA_ADD_FLAT_MODIFIER = 107,                                 // Add Flat Modifier
    SPELL_AURA_ADD_PCT_MODIFIER = 108,                                  // Add % Modifier
    SPELL_AURA_ADD_CLASS_TARGET_TRIGGER = 109,                          // Add Class Target Trigger
    SPELL_AURA_MOD_POWER_REGEN_PERCENT = 110,                           // Mod Power Regen %
    SPELL_AURA_ADD_CASTER_HIT_TRIGGER = 111,                            // Add Class Caster Hit Trigger
    SPELL_AURA_OVERRIDE_CLASS_SCRIPTS = 112,                            // Override Class Scripts
    SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN = 113,                           // Mod Ranged Dmg Taken
    SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN_PCT = 114,                       // Mod Ranged % Dmg Taken
    SPELL_AURA_MOD_HEALING = 115,                                       // Mod Healing
    SPELL_AURA_IGNORE_REGEN_INTERRUPT = 116,                            // Regen During Combat
    SPELL_AURA_MOD_MECHANIC_RESISTANCE = 117,                           // Mod Mechanic Resistance
    SPELL_AURA_MOD_HEALING_PCT = 118,                                   // Mod Healing %
    SPELL_AURA_SHARE_PET_TRACKING = 119,                                // Share Pet Tracking
    SPELL_AURA_UNTRACKABLE = 120,                                       // Untrackable
    SPELL_AURA_EMPATHY = 121,                                           // Empathy (Lore, whatever)
    SPELL_AURA_MOD_OFFHAND_DAMAGE_PCT = 122,                            // Mod Offhand Dmg %
    SPELL_AURA_MOD_POWER_COST_PCT = 123,                                // Mod Power Cost % --> armor penetration & spell penetration
    SPELL_AURA_MOD_RANGED_ATTACK_POWER = 124,                           // Mod Ranged Attack Power
    SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN = 125,                            // Mod Melee Dmg Taken
    SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN_PCT = 126,                        // Mod Melee % Dmg Taken
    SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS = 127,                // Rngd Atk Pwr Attckr Bonus
    SPELL_AURA_MOD_POSSESS_PET = 128,                                   // Mod Possess Pet
    SPELL_AURA_MOD_INCREASE_SPEED_ALWAYS = 129,                         // Mod Speed Always
    SPELL_AURA_MOD_MOUNTED_SPEED_ALWAYS = 130,                          // Mod Mounted Speed Always
    SPELL_AURA_MOD_CREATURE_RANGED_ATTACK_POWER = 131,                  // Mod Creature Ranged Attack Power
    SPELL_AURA_MOD_INCREASE_ENERGY_PERCENT = 132,                       // Mod Increase Energy %
    SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT = 133,                       // Mod Max Health %
    SPELL_AURA_MOD_MANA_REGEN_INTERRUPT = 134,                          // Mod Interrupted Mana Regen
    SPELL_AURA_MOD_HEALING_DONE = 135,                                  // Mod Healing Done
    SPELL_AURA_MOD_HEALING_DONE_PERCENT = 136,                          // Mod Healing Done %
    SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE = 137,                         // Mod Total Stat %
    SPELL_AURA_MOD_HASTE = 138,                                         // Haste - Melee
    SPELL_AURA_FORCE_REACTION = 139,                                    // Force Reaction
    SPELL_AURA_MOD_RANGED_HASTE = 140,                                  // Haste - Ranged
    SPELL_AURA_MOD_RANGED_AMMO_HASTE = 141,                             // Haste - Ranged (Ammo Only)
    SPELL_AURA_MOD_BASE_RESISTANCE_PCT = 142,                           // Mod Base Resistance %
    SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE = 143,                          // Mod Resistance Exclusive
    SPELL_AURA_SAFE_FALL = 144,                                         // Safe Fall
    SPELL_AURA_CHARISMA = 145,                                          // Charisma
    SPELL_AURA_PERSUADED = 146,                                         // Persuaded
    SPELL_AURA_MECHANIC_IMMUNITY_MASK = 147,                            // Mechanic Immunity Mask
    SPELL_AURA_RETAIN_COMBO_POINTS = 148,                               // Retain Combo Points
    SPELL_AURA_RESIST_PUSHBACK = 149,                                   // Resist Pushback
    SPELL_AURA_MOD_SHIELD_BLOCK_PCT = 150,                              // Mod Shield Block %
    SPELL_AURA_TRACK_STEALTHED = 151,                                   // Track Stealthed
    SPELL_AURA_MOD_DETECTED_RANGE = 152,                                // Mod Detected Range
    SPELL_AURA_SPLIT_DAMAGE_FLAT = 153,                                 // Split Damage Flat
    SPELL_AURA_MOD_STEALTH_LEVEL = 154,                                 // Stealth Level Modifier
    SPELL_AURA_MOD_WATER_BREATHING = 155,                               // Mod Water Breathing
    SPELL_AURA_MOD_REPUTATION_ADJUST = 156,                             // Mod Reputation Gain
    SPELL_AURA_PET_DAMAGE_MULTI = 157,                                  // Mod Pet Damage
    SPELL_AURA_MOD_SHIELD_BLOCK = 158,                                  // Mod Shield Block
    SPELL_AURA_NO_PVP_CREDIT = 159,                                     // No PVP Credit
    SPELL_AURA_MOD_SIDE_REAR_PDAE_DAMAGE_TAKEN = 160,                   // Mod Side/Rear PBAE Damage Taken
    SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT = 161,                        // Mod Health Regen In Combat
    SPELL_AURA_POWER_BURN = 162,                                        // Power Burn
    SPELL_AURA_MOD_CRIT_DAMAGE_BONUS_MELEE = 163,                       // Mod Critical Damage Bonus (Physical)
    SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS = 165,                 // Melee AP Attacker Bonus
    SPELL_AURA_MOD_ATTACK_POWER_PCT = 166,                              // Mod Attack Power
    SPELL_AURA_MOD_RANGED_ATTACK_POWER_PCT = 167,                       // Mod Ranged Attack Power %
    SPELL_AURA_INCREASE_DAMAGE = 168,                                   // Increase Damage Type
    SPELL_AURA_INCREASE_CRITICAL = 169,                                 // Increase Critical Type
    SPELL_AURA_DETECT_AMORE = 170,                                      // Detect Amore
    SPELL_AURA_INCREASE_MOVEMENT_AND_MOUNTED_SPEED = 172,               // Increase Movement and Mounted Speed (Non-Stacking)
    SPELL_AURA_INCREASE_SPELL_DAMAGE_PCT = 174,                         // Increase Spell Damage by % status
    SPELL_AURA_INCREASE_SPELL_HEALING_PCT = 175,                        // Increase Spell Healing by % status
    SPELL_AURA_SPIRIT_OF_REDEMPTION = 176,                              // Spirit of Redemption Auras
    SPELL_AURA_AREA_CHARM = 177,                                        // Area Charm
    SPELL_AURA_INCREASE_ATTACKER_SPELL_CRIT = 179,                      // Increase Attacker Spell Crit Type
    SPELL_AURA_INCREASE_SPELL_DAMAGE_VS_TYPE = 180,                     // Increase Spell Damage Type
    SPELL_AURA_INCREASE_ARMOR_BASED_ON_INTELLECT_PCT = 182,             // Increase Armor based on Intellect
    SPELL_AURA_DECREASE_CRIT_THREAT = 183,                              // Decrease Critical Threat by
    SPELL_AURA_DECREASE_ATTACKER_CHANCE_TO_HIT_MELEE = 184,             // Reduces Attacker Chance to Hit with Melee
    SPELL_AURA_DECREASE_ATTACKER_CHANGE_TO_HIT_RANGED = 185,            // Reduces Attacker Chance to Hit with Ranged
    SPELL_AURA_DECREASE_ATTACKER_CHANGE_TO_HIT_SPELLS = 186,            // Reduces Attacker Chance to Hit with Spells
    SPELL_AURA_DECREASE_ATTACKER_CHANGE_TO_CRIT_MELEE = 187,            // Reduces Attacker Chance to Crit with Melee (Ranged?)
    SPELL_AURA_DECREASE_ATTACKER_CHANGE_TO_CRIT_RANGED = 188,           // Reduces Attacker Chance to Crit with Ranged (Melee?)
    SPELL_AURA_INCREASE_REPUTATION = 190,                               // Increases reputation from killed creatures
    SPELL_AURA_SPEED_LIMIT = 191,                                       // speed limit
    SPELL_AURA_MELEE_SLOW_PCT = 192,
    SPELL_AURA_INCREASE_TIME_BETWEEN_ATTACKS = 193,
    SPELL_AURA_INREASE_SPELL_DAMAGE_PCT_OF_INTELLECT = 194,             // NOT USED ANYMORE - 174 used instead
    SPELL_AURA_INCREASE_HEALING_PCT_OF_INTELLECT = 195,		            // NOT USED ANYMORE - 175 used instead
    SPELL_AURA_MOD_ALL_WEAPON_SKILLS = 196,
    SPELL_AURA_REDUCE_ATTACKER_CRICTICAL_HIT_CHANCE_PCT = 197,
    SPELL_AURA_198 = 198,
    SPELL_AURA_INCREASE_SPELL_HIT_PCT = 199,
    SPELL_AURA_FLY = 201,
    SPELL_AURA_FINISHING_MOVES_CANNOT_BE_DODGED = 202,
    SPELL_AURA_REDUCE_ATTACKER_CRICTICAL_HIT_DAMAGE_MELEE_PCT = 203,
    SPELL_AURA_REDUCE_ATTACKER_CRICTICAL_HIT_DAMAGE_RANGED_PCT = 204,
    SPELL_AURA_ENABLE_FLIGHT = 206,
    SPELL_AURA_ENABLE_FLIGHT2 = 207,
    SPELL_AURA_ENABLE_FLIGHT_WITH_UNMOUNTED_SPEED = 208,
    SPELL_AURA_MOD_RANGED_ATTACK_POWER_BY_STAT_PCT = 212,
    SPELL_AURA_INCREASE_RAGE_FROM_DAMAGE_DEALT_PCT = 213,
    SPELL_AURA_INCREASE_CASTING_TIME_PCT = 216,
    SPELL_AURA_REGEN_MANA_STAT_PCT = 219,
    SPELL_AURA_HEALING_STAT_PCT = 220,
    SPELL_AURA_PERIODIC_TRIGGER_DUMMY = 226,
    SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE = 227,                 // Used by Mind Flay etc
    SPELL_AURA_DETECT_STEALTH = 228,
    SPELL_AURA_REDUCE_AOE_DAMAGE_TAKEN = 229,
    SPELL_AURA_INCREASE_MAX_HEALTH = 230,                               //Used by Commanding Shout
    SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE = 231,
    SPELL_AURA_MECHANIC_DURATION_MOD = 232,
    SPELL_AURA_MOD_HEALING_BY_AP = 237,
    SPELL_AURA_MOD_SPELL_DAMAGE_BY_AP = 238,
    SPELL_AURA_EXPERTISE = 240,
    SPELL_AURA_FORCE_MOVE_FORWARD = 241,
    SPELL_AURA_MOD_SPELL_DAMAGE_FROM_HEALING = 242,
    SPELL_AURA_243 = 243,
    SPELL_AURA_COMPREHEND_LANG = 244,
    SPELL_AURA_MOD_DURATION_OF_MAGIC_EFFECTS = 245,
    SPELL_AURA_246 = 246,
    SPELL_AURA_247 = 247,
    SPELL_AURA_MOD_COMBAT_RESULT_CHANCE = 248,
    SPELL_AURA_CONVERT_RUNE = 249,
    SPELL_AURA_MOD_INCREASE_HEALTH_2 = 250,
    SPELL_AURA_MOD_ENEMY_DODGE = 251,
    SPELL_AURA_252 = 252,
    SPELL_AURA_BLOCK_MULTIPLE_DAMAGE = 253,
    SPELL_AURA_MOD_DISARM_OFFHAND = 254,
    SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT = 255,
    SPELL_AURA_256 = 256,
    SPELL_AURA_257 = 257,
    SPELL_AURA_258 = 258,
    SPELL_AURA_259 = 259,
    SPELL_AURA_260 = 260,
    SPELL_AURA_PHASE = 261,
    SPELL_AURA_IGNORE_TARGET_AURA_STATE = 262,
    SPELL_AURA_ALLOW_ONLY_ABILITY = 263,
    SPELL_AURA_264 = 264,
    SPELL_AURA_265 = 265,
    SPELL_AURA_266 = 266,
    SPELL_AURA_267 = 267,
    SPELL_AURA_MOD_ATTACK_POWER_BY_STAT_PCT = 268,
    SPELL_AURA_269 = 269,
    SPELL_AURA_270 = 270,
    SPELL_AURA_INCREASE_SPELL_DOT_DAMAGE_PCT = 271,
    SPELL_AURA_272 = 272,
    SPELL_AURA_273 = 273,
    SPELL_AURA_CONSUMES_NO_AMMO = 274,
    SPELL_AURA_IGNORE_SHAPESHIFT = 275,
    SPELL_AURA_276 = 276,
    SPELL_AURA_277 = 277,
    SPELL_AURA_MOD_DISARM_RANGED = 278,
    SPELL_AURA_279 = 279,
    SPELL_AURA_IGNORE_ARMOR_PCT = 280,
    SPELL_AURA_281 = 281,
    SPELL_AURA_MOD_BASE_HEALTH = 282,
    SPELL_AURA_283 = 283,
    SPELL_AURA_284 = 284,
    SPELL_AURA_MOD_ATTACK_POWER_OF_ARMOR = 285,
    SPELL_AURA_ALLOW_DOT_TO_CRIT = 286,
    SPELL_AURA_DEFLECT_SPELLS = 287,
    SPELL_AURA_288 = 288,
    SPELL_AURA_289 = 289,
    SPELL_AURA_290 = 290,
    SPELL_AURA_MOD_XP_QUEST_PCT = 291,
    SPELL_AURA_292 = 292,
    SPELL_AURA_293 = 293,
    SPELL_AURA_294 = 294,
    SPELL_AURA_295 = 295,
    SPELL_AURA_296 = 296,
    SPELL_AURA_297 = 297,
    SPELL_AURA_298 = 298,
    SPELL_AURA_299 = 299,
    SPELL_AURA_300 = 300,
    SPELL_AURA_301 = 301,
    SPELL_AURA_302 = 302,
    SPELL_AURA_303 = 303,
    SPELL_AURA_304 = 304,
    SPELL_AURA_305 = 305,
    SPELL_AURA_306 = 306,
    SPELL_AURA_307 = 307,
    SPELL_AURA_308 = 308,
    SPELL_AURA_309 = 309,
    SPELL_AURA_310 = 310,
    SPELL_AURA_311 = 311,
    SPELL_AURA_312 = 312,
    SPELL_AURA_313 = 313,
    SPELL_AURA_PREVENT_RESURRECTION = 314,
    SPELL_AURA_315 = 315,
    SPELL_AURA_316 = 316,
    TOTAL_SPELL_AURAS = 317,
};


#define MAX_SPELL_EFFECTS 3
#define MAX_SPELL_TOTEMS 2
#define MAX_SPELL_TOTEM_CATEGORIES 2
#define MAX_SPELL_REAGENTS 8
#define MAX_SPELL_ID 121820
