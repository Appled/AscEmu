/*
Copyright (c) 2014-2026 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#pragma once

#include "Server/ClientProtocol.hpp"

#include <cstdint>

namespace AuraSlots
{
    constexpr uint16_t PassiveSlotStart = 0;
    constexpr uint16_t TotalSlotStart = 0;
    constexpr uint16_t PositiveVisualSlotStart = 0;

    inline uint16_t getPassiveSlotEnd() noexcept
    {
        // 100 passive aura slots for each unit in Classic
        if (WoW::isServerExpansion(WoW::Expansion::_Classic))
            return PassiveSlotStart + 100;
        // 170 passive aura slots for each unit in TBC
        if (WoW::isServerExpansion(WoW::Expansion::_TBC))
            return PassiveSlotStart + 170;
        // 180 passive aura slots for each unit in other versions
        return PassiveSlotStart + 180;
    }

    inline uint16_t getPositiveSlotStart() noexcept
    {
        return getPassiveSlotEnd();
    }

    inline uint16_t getPositiveSlotEnd() noexcept
    {
        // 32 helpful aura slots for each unit in Classic
        if (WoW::isServerExpansion(WoW::Expansion::_Classic))
            return getPositiveSlotStart() + 32;
        // 40 helpful aura slots for each unit in TBC
        if (WoW::isServerExpansion(WoW::Expansion::_TBC))
            return getPositiveSlotStart() + 40;
        // 80 helpful aura slots for each unit in other versions
        return getPositiveSlotStart() + 80;
    }

    inline uint16_t getNegativeSlotStart() noexcept
    {
        return getPositiveSlotEnd();
    }

    inline uint16_t getNegativeSlotEnd() noexcept
    {
        // 16 harmful aura slots for each unit in Classic
        if (WoW::isServerExpansion(WoW::Expansion::_Classic))
            return getNegativeSlotStart() + 16;
        // 40 harmful aura slots for each unit in TBC
        if (WoW::isServerExpansion(WoW::Expansion::_TBC))
            return getNegativeSlotStart() + 40;
        // 100 harmful aura slots for each unit in other versions
        return getNegativeSlotStart() + 100;
    }

    inline uint16_t getRemovableSlotStart() noexcept
    {
        return getPositiveSlotStart();
    }

    inline uint16_t getRemovableSlotEnd() noexcept
    {
        return getNegativeSlotEnd();
    }

    inline uint16_t getTotalSlotEnd() noexcept
    {
        return getNegativeSlotEnd();
    }

    // These refer to visual slots (= auras visible on client)

    inline uint8_t getPositiveVisualSlotEnd() noexcept
    {
        // 32 helpful visual aura slots for each unit in Classic
        if (WoW::isServerExpansion(WoW::Expansion::_Classic))
            return PositiveVisualSlotStart + 32;
        // 40 helpful visual aura slots for each unit in other versions
        return PositiveVisualSlotStart + 40;
    }

    inline uint8_t getNegativeVisualSlotStart() noexcept
    {
        return getPositiveVisualSlotEnd();
    }

    inline uint8_t getNegativeVisualSlotEnd() noexcept
    {
        // 16 harmful visual aura slots for each unit in all versions
        return getNegativeVisualSlotStart() + 16;
    }
};
