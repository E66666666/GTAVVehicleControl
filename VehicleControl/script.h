#pragma once
#include <inc/types.h>

#define DISPLAY_VERSION "v1.0.0"

const char* const modDir  = "\\VehicleControl";

void ScriptMain();
void onMain();
void onExit();
void update_menu();

class ManagedVehicle {
public:
    ManagedVehicle(Vehicle vehicle) : Vehicle(vehicle),
                                      DoorIndex(0),
                                      BlinkerIndex(0) {}
    bool operator==(const ManagedVehicle& rhs) {
        return rhs.Vehicle == this->Vehicle;
    }
    Vehicle Vehicle;
    int DoorIndex;
    int BlinkerIndex;
    int BombBayIndex;
};
