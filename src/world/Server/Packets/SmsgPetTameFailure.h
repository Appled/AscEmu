/*
Copyright (c) 2014-2026 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#pragma once

#include "ManagedPacket.h"
#include "Spell/Definitions/SpellFailure.hpp"

#include <cstdint>

namespace AscEmu::Packets
{
    class SmsgPetTameFailure : public ManagedPacket
    {
    public:
        uint8_t reason;

        SmsgPetTameFailure() : SmsgPetTameFailure(0)
        {
        }

        SmsgPetTameFailure(uint8_t reason) :
            ManagedPacket(SMSG_PET_TAME_FAILURE, 4),
            reason(reason)
        {
        }

    protected:
        size_t expectedSize() const override { return 4; }

        bool internalSerialise(WorldPacket& packet) override
        {
            if (m_protocol.expansion <= WoW::Expansion::_Cata)
            {
                auto wireReason = reason;
                if (reason == PETTAME_UNKNOWNERROR)
                {
                    wireReason = m_protocol.expansion <= WoW::Expansion::_WotLK ?
                        PETTAME_UNKNOWNERROR_PRE_CATA :
                        PETTAME_UNKNOWNERROR_POST_CATA;
                }

                packet << wireReason;
                return true;
            }

            return false;
        }

        bool internalDeserialise(WorldPacket& /*packet*/) override { return false; }
    };
}
