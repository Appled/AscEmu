/*
Copyright (c) 2014-2020 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#include "Setup.h"

#if VERSION_STRING >= WotLK
enum DeathknightSpells
{
    SPELL_BUTCHERY_ENERGIZE             = 50163,
    SPELL_BUTCHERY_R1                   = 48979,
    SPELL_BUTCHERY_R2                   = 49483,
    SPELL_DEATH_RUNE_MASTERY_BLOOD      = 50806,
    SPELL_DEATH_RUNE_MASTERY_R1         = 49467,
    SPELL_DEATH_RUNE_MASTERY_R2         = 50033,
    SPELL_DEATH_RUNE_MASTERY_R3         = 50034,
    SPELL_MARK_OF_BLOOD_DUMMY           = 49005,
    SPELL_MARK_OF_BLOOD_HEAL            = 61607,
};

#if VERSION_STRING < Mop
class Butchery : public SpellScript
{
public:
    SpellScriptCheckDummy onAuraDummyEffect(Aura* aur, AuraEffectModifier* aurEff, bool apply) override
    {
        if (!apply)
            return SpellScriptCheckDummy::DUMMY_OK;

        auto spellProc = aur->getOwner()->addProcTriggerSpell(sSpellMgr.getSpellInfo(SPELL_BUTCHERY_ENERGIZE), aur->getSpellInfo(), aur->getCasterGuid(), 0);
        if (spellProc != nullptr)
            spellProc->setOverrideEffectDamage(EFF_INDEX_0, aurEff->getEffectDamage());

        return SpellScriptCheckDummy::DUMMY_OK;
    }

    void onAuraRemove(Aura* aur, AuraRemoveMode /*mode*/) override
    {
        aur->getOwner()->removeProcTriggerSpell(SPELL_BUTCHERY_ENERGIZE, aur->getCasterGuid());
    }
};
#endif

#if VERSION_STRING == WotLK
class DeathRuneMastery : public SpellScript
{
 public:
     void onAuraApply(Aura* aur) override
     {
         // Should proc only on Obliterate and Death Strike
         uint32_t procFamilyMask[3] = { 0x10, 0x20000, 0 };
         aur->getOwner()->addProcTriggerSpell(sSpellMgr.getSpellInfo(SPELL_DEATH_RUNE_MASTERY_BLOOD), aur->getSpellInfo(), aur->getCasterGuid(), 0, procFamilyMask);
     }

     void onAuraRemove(Aura* aur, AuraRemoveMode /*mode*/) override
     {
         aur->getOwner()->removeProcTriggerSpell(SPELL_DEATH_RUNE_MASTERY_BLOOD, aur->getCasterGuid());
     }
};

class MarkOfBlood : public SpellScript
{
public:
    SpellScriptCheckDummy onAuraDummyEffect(Aura* aur, AuraEffectModifier* /*aurEff*/, bool apply) override
    {
        if (!apply)
            return SpellScriptCheckDummy::DUMMY_OK;

        aur->getOwner()->addProcTriggerSpell(sSpellMgr.getSpellInfo(SPELL_MARK_OF_BLOOD_HEAL), aur->getSpellInfo(), aur->getCasterGuid(), 0);
        return SpellScriptCheckDummy::DUMMY_OK;
    }

    void onAuraRemove(Aura* aur, AuraRemoveMode /*mode*/) override
    {
        aur->getOwner()->removeProcTriggerSpell(SPELL_MARK_OF_BLOOD_HEAL, aur->getCasterGuid());
    }
};
#endif

void setupDeathKnightSpells(ScriptMgr* mgr)
{
    // Call legacy script setup
    SetupLegacyDeathKnightSpells(mgr);

#if VERSION_STRING < Mop
    uint32_t butcheryIds[] =
    {
        SPELL_BUTCHERY_R1,
        SPELL_BUTCHERY_R2,
        0
    };
    mgr->register_spell_script(butcheryIds, new Butchery);
#endif

#if VERSION_STRING == WotLK
    uint32_t deathRuneMasteryIds[] =
    {
        SPELL_DEATH_RUNE_MASTERY_R1,
        SPELL_DEATH_RUNE_MASTERY_R2,
        SPELL_DEATH_RUNE_MASTERY_R3,
        0
    };
    mgr->register_spell_script(deathRuneMasteryIds, new DeathRuneMastery);

    mgr->register_spell_script(SPELL_MARK_OF_BLOOD_DUMMY, new MarkOfBlood);
#endif
}
#endif
