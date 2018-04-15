#pragma once
#include <inc/types.h>

#define DISPLAY_VERSION "v1.0.0"

const char* const modDir  = "\\VehicleControl";

void ScriptMain();
void onMain();
void onExit();
void update_menu();
void initRadioStations();

struct Neon {
    bool Left;
    bool Right;
    bool Front;
    bool Rear;
};

class ManagedVehicle {
public:
    ManagedVehicle(Vehicle vehicle, int radioIdx)
        : Vehicle(vehicle),
          DoorIndex(0),
          BlinkerIndex(0),
          BombBayIndex(0),
          RadioIndex(radioIdx),
          WindowIndex(0),
          NeonIndex(0) { }

    bool operator==(const ManagedVehicle& rhs) {
        return rhs.Vehicle == this->Vehicle;
    }

    Vehicle Vehicle;
    int DoorIndex;
    int BlinkerIndex;
    int BombBayIndex;
    int RadioIndex;
    int WindowIndex;
    int NeonIndex;
};
