#pragma once
#include <inc/types.h>

#define DISPLAY_VERSION "v1.1.0"

const char* const modDir  = "\\VehicleControl";

void ScriptMain();
void onMain();
void onExit();
void update_menu();
void initRadioStations();

enum class WindowState {
    Up,
    Down,
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
          NeonOn(true) {
        WindowState[0] = WindowState::Up;
        WindowState[1] = WindowState::Up;
        WindowState[2] = WindowState::Up;
        WindowState[3] = WindowState::Up;
    }

    bool operator==(const ManagedVehicle& rhs) {
        return rhs.Vehicle == this->Vehicle;
    }

    Vehicle Vehicle;
    int DoorIndex;
    int BlinkerIndex;
    int BombBayIndex;
    int RadioIndex;
    int WindowIndex;
    bool NeonOn;
    WindowState WindowState[4];
};
