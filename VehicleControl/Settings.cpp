#include "Settings.h"

// This comes from GTAVMenuBase. Horrible, I know.
#include <thirdparty/simpleini/SimpleIni.h>

void Settings::Load() {
    CSimpleIniA settings;
    settings.LoadFile("VehicleControl/settings.ini");

    MPAudio = settings.GetBoolValue("Compatibility", "MPAudio", MPAudio);

    OffsetSteerAngle = settings.GetLongValue("Offsets", "OffsetSteerAngle", OffsetSteerAngle);
    OffsetSteerInput = settings.GetLongValue("Offsets", "OffsetSteerInput", OffsetSteerInput);
    OffsetHandling = settings.GetLongValue("Offsets", "OffsetHandling", OffsetHandling);
    OffsetHandlingSteeringAngle = settings.GetLongValue("Offsets", "OffsetHandlingSteeringAngle", OffsetHandlingSteeringAngle);
}
