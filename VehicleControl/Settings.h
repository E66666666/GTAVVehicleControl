#pragma once
#include <cstdint>

class Settings
{
public:
    void Load();

    // b1868
    uint32_t OffsetSteerAngle = 0x994;
    uint32_t OffsetSteerInput = 0x98C;
    uint32_t OffsetHandling = 0x918;
    uint32_t OffsetHandlingSteeringAngle = 0x80;
    bool MPAudio = true;
};

