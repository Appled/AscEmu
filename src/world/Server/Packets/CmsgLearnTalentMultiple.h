/*
Copyright (c) 2014-2018 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#pragma once

#include <cstdint>

#include "ManagedPacket.h"
#include "WorldPacket.h"

struct MultipleTalents
{
    uint32_t talentId;
    uint32_t talentRank;
};

namespace AscEmu { namespace Packets
{
    class CmsgLearnTalentMultiple : public ManagedPacket
    {
    public:
#if VERSION_STRING == Cata
        int32_t currentTab;
#endif
        uint32_t talentCount;
        std::vector<MultipleTalents> multipleTalents;

        CmsgLearnTalentMultiple() : CmsgLearnTalentMultiple(0, {})
        {
        }

        CmsgLearnTalentMultiple(uint32_t talentCount, std::vector<MultipleTalents> multipleTalents) :
            ManagedPacket(CMSG_LEARN_TALENTS_MULTIPLE, 4),
            talentCount(talentCount),
            multipleTalents(multipleTalents)
        {
        }

    protected:
        bool internalSerialise(WorldPacket& packet) override
        {
            return false;
        }

        bool internalDeserialise(WorldPacket& packet) override
        {
#if VERSION_STRING == Cata
            packet >> currentTab;
#endif
            packet >> talentCount;

            MultipleTalents multiTalent{};

            for (uint32_t i = 0; i < talentCount; ++i)
            {
                packet >> multiTalent.talentId >> multiTalent.talentRank;

                multipleTalents.push_back(multiTalent);
            }
            return true;
        }
    };
}}