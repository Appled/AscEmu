/****************************************************************************
 *
 * SpellHandler Plugin
 * Copyright (c) 2007 Team Ascent
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0
 * License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons,
 * 543 Howard Street, 5th Floor, San Francisco, California, 94105, USA.
 *
 * EXCEPT TO THE EXTENT REQUIRED BY APPLICABLE LAW, IN NO EVENT WILL LICENSOR BE LIABLE TO YOU
 * ON ANY LEGAL THEORY FOR ANY SPECIAL, INCIDENTAL, CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES
 * ARISING OUT OF THIS LICENSE OR THE USE OF THE WORK, EVEN IF LICENSOR HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 *
 */

#include "Setup.h"
#define SKIP_ALLOCATOR_SHARING 1
#include <Server/Script/ScriptSetup.h>
#include "Server/Script/ScriptMgr.h"

extern "C" SCRIPT_DECL void _exp_set_serverstate_singleton(ServerState* state)
{
    ServerState::instance(state);
}

extern "C" SCRIPT_DECL uint32 _exp_get_script_type()
{
    return SCRIPT_TYPE_MISC;
}

extern "C" SCRIPT_DECL void _exp_script_register(ScriptMgr* mgr)
{
    SetupShamanSpells(mgr);
    SetupWarlockSpells(mgr);
    SetupWarriorSpells(mgr);
    SetupHunterSpells(mgr);
    SetupItemSpells_1(mgr);
    SetupQuestItems(mgr); //this was commented for crash reason, let see what are those...
    SetupMageSpells(mgr);
    SetupPaladinSpells(mgr);
    SetupRogueSpells(mgr);
    SetupPriestSpells(mgr);
    SetupPetAISpells(mgr);
    SetupDruidSpells(mgr);
    SetupDeathKnightSpells(mgr);
    SetupMiscSpellhandlers(mgr);
}

#ifdef WIN32

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}

#endif

/*
DONT REMOVE THESE, TEMPORARY LOCATION FOR ALL HACKFIXES FOUND IN OLD SPELL SYSTEM, EVERYTHING WILL BE CONVERTED TO SPELL SCRIPT!! -Appled

if (m_spellInfo->custom_NameHash == SPELL_HASH_DEATH_PACT && target->GetSummonedByGUID() != m_caster->GetGUID())
return SPELL_FAILED_BAD_TARGETS;


        if (m_spellInfo->Id == 32146)
        {
            Creature* corpse = m_caster->GetMapMgr()->GetInterface()->GetCreatureNearestCoords(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), 18240);
            if (corpse != NULL)
                if (m_caster->CalcDistance(m_caster, corpse) > 5)
                    return SPELL_FAILED_NOT_HERE;
        }
        else if (m_spellInfo->Id == 39246)
        {
            Creature* cleft = m_caster->GetMapMgr()->GetInterface()->GetCreatureNearestCoords(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), 22105);
            if (cleft == NULL || cleft->isAlive())
                return SPELL_FAILED_NOT_HERE;
        }
        else if (m_spellInfo->Id == 30988)
        {
            Creature* corpse = m_caster->GetMapMgr()->GetInterface()->GetCreatureNearestCoords(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), 17701);
            if (corpse != NULL)
                if (m_caster->CalcDistance(m_caster, corpse) > 5 || corpse->isAlive())
                    return SPELL_FAILED_NOT_HERE;
        }
        else if (m_spellInfo->Id == 43723)
        {
            Creature* abysal = p_caster->GetMapMgr()->GetInterface()->GetCreatureNearestCoords(p_caster->GetPositionX(), p_caster->GetPositionY(), p_caster->GetPositionZ(), 19973);
            if (abysal != NULL)
            {
                if (!abysal->isAlive())
                    if (!(p_caster->GetItemInterface()->GetItemCount(31672) > 1 && p_caster->GetItemInterface()->GetItemCount(31673) > 0 && p_caster->CalcDistance(p_caster, abysal) < 10))
                        return SPELL_FAILED_NOT_HERE;
            }
            else
                return SPELL_FAILED_NOT_HERE;
        }
        else if (m_spellInfo->Id == 32307)
        {
            Creature* kilsorrow = p_caster->GetMapMgr()->GetInterface()->GetCreatureNearestCoords(p_caster->GetPositionX(), p_caster->GetPositionY(), p_caster->GetPositionZ());
            if (kilsorrow == NULL || kilsorrow->isAlive() || p_caster->CalcDistance(p_caster, kilsorrow) > 1)
                return SPELL_FAILED_NOT_HERE;
            if (kilsorrow->GetEntry() != 17147 && kilsorrow->GetEntry() != 17148 && kilsorrow->GetEntry() != 18397 && kilsorrow->GetEntry() != 18658 && kilsorrow->GetEntry() != 17146)
                return SPELL_FAILED_NOT_HERE;
        }


    switch (Id)
    {
        case 23333:                                         // Warsong Flag
        case 23335:                                         // Silverwing Flag
            return map_id == 489 && player && player->InBattleground() ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        case 34976:                                         // Netherstorm Flag
            return map_id == 566 && player && player->InBattleground() ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        case 2584:                                          // Waiting to Resurrect
        case 22011:                                         // Spirit Heal Channel
        case 22012:                                         // Spirit Heal
        case 24171:                                         // Resurrection Impact Visual
        case 42792:                                         // Recently Dropped Flag
        case 43681:                                         // Inactive
        case 44535:                                         // Spirit Heal (mana)
        {
            MapEntry const* mapEntry = sMapStore.LookupEntry(map_id);
            if (!mapEntry)
                return SPELL_FAILED_INCORRECT_AREA;

            return zone_id == 4197 || (mapEntry->IsBattleground() && player && player->InBattleground()) ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        }
        case 44521:                                         // Preparation
        {
            if (!player)
                return SPELL_FAILED_REQUIRES_AREA;

            MapEntry const* mapEntry = sMapStore.LookupEntry(map_id);
            if (!mapEntry)
                return SPELL_FAILED_INCORRECT_AREA;

            if (!mapEntry->IsBattleground())
                return SPELL_FAILED_REQUIRES_AREA;

            Battleground* bg = player->GetBattleground();
            return bg && bg->GetStatus() == STATUS_WAIT_JOIN ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        }
        case 32724:                                         // Gold Team (Alliance)
        case 32725:                                         // Green Team (Alliance)
        case 35774:                                         // Gold Team (Horde)
        case 35775:                                         // Green Team (Horde)
        {
            MapEntry const* mapEntry = sMapStore.LookupEntry(map_id);
            if (!mapEntry)
                return SPELL_FAILED_INCORRECT_AREA;

            return mapEntry->IsBattleArena() && player && player->InBattleground() ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        }
        case 32727:                                         // Arena Preparation
        {
            if (!player)
                return SPELL_FAILED_REQUIRES_AREA;

            MapEntry const* mapEntry = sMapStore.LookupEntry(map_id);
            if (!mapEntry)
                return SPELL_FAILED_INCORRECT_AREA;

            if (!mapEntry->IsBattleArena())
                return SPELL_FAILED_REQUIRES_AREA;

            Battleground* bg = player->GetBattleground();
            return bg && bg->GetStatus() == STATUS_WAIT_JOIN ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        }
    }


    PAIN SUPPRESSION, should not be casted while stunned. only possible with the glyph.

    SPELL IDS 23336, 23334 and 34991, (battleground flag drops) should be force casted, regardless of caster's state

    WARLOCK IMP FIRE SHIELD, MAGE FOCUS MAGIC cannot be cast on self

    LAY ON HANDS only self cast, check for forbearance and avenging wrath marker

    CONFLAGRATE, ENVENOM, requires main aura to be casted by caster
*/
