#pragma once
#include "inc/types.h"
#include "Blip.h"

enum class WindowState {
    Up,
    Down,
};

class ManagedVehicle {
public:
    ManagedVehicle(Vehicle vehicle, int radioIdx)
        : Vehicle(vehicle)
        , DoorIndex(0)
        , BlinkerIndex(0)
        , BombBayIndex(0)
        , RadioIndex(radioIdx)
        , WindowIndex(0)
        , NeonOn(true)
        , WindowState{
            WindowState::Up,
            WindowState::Up,
            WindowState::Up,
            WindowState::Up
        }
        , Blip(nullptr) { }

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
    Blip_* Blip;
};
