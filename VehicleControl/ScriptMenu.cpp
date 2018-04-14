#include "script.h"

#include <menu.h>
#include "Util/StringFormat.h"
#include "Util/UIUtils.h"

using namespace std;

extern NativeMenu::Menu menu;

extern Player player;
extern Ped playerPed;
extern Vehicle currentVehicle;
extern Vehicle lastVehicle;

extern vector<Vehicle> managedVehicles;

const int NumDoors = 6;

enum class LockStatus : int {
    None,
    Unlocked,
    Locked,
    LockedForPlayer,
    StickPlayerInside,
    CanBeBrokenInto = 7,
    CanBeBrokenIntoPersist,
    CannotBeTriedToEnter = 10
};

vector<LockStatus> LockStatuses {
    LockStatus::Unlocked,
    LockStatus::Locked,
};

unordered_map<LockStatus, string> LockStatusText {
    { LockStatus::None,                     "None" },
    { LockStatus::Unlocked,                 "Unlocked" },
    { LockStatus::Locked,                   "Locked" },
    { LockStatus::LockedForPlayer,          "LockedForPlayer" },
    { LockStatus::StickPlayerInside,        "StickPlayerInside" },
    { LockStatus::CanBeBrokenInto,          "CanBeBrokenInto" },
    { LockStatus::CanBeBrokenIntoPersist,   "CanBeBrokenIntoPersist" },
    { LockStatus::CannotBeTriedToEnter,     "CannotBeTriedToEnter" }, 
};

enum class Door {
    FrontLeftDoor = 0,
    FrontRightDoor = 1,
    BackLeftDoor = 2,
    BackRightDoor = 3,
    Hood = 4,
    Trunk = 5
};

vector<Door> Doors{
    Door::FrontLeftDoor,
    Door::FrontRightDoor,
    Door::BackLeftDoor,
    Door::BackRightDoor,
    Door::Hood,
    Door::Trunk,
};

vector<string> VehicleDoorText {
    "Front Left",
    "Front Right",
    "Back Left",
    "Back Right",
    "Hood",
    "Trunk",
    "All Doors"
};

vector<string> GenericOnOff {
    "Off",
    "On"
};

int currentVehicleIndex = 0;
int currentDoorIndex = 0;
int currentHazard = 0;

void onMain() {
    menu.ReadSettings();
}

void onExit() {

}

void onRight() {

}

void onLeft() {

}

string getGxtName(Hash hash) {
    char *name = VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(hash);
    string displayName = UI::_GET_LABEL_TEXT(name);
    if (displayName == "NULL") {
        displayName = name;
    }
    return displayName;
}

pair<string, string> GetVehicleNames(Vehicle vehicle) {
    Hash modelHash = ENTITY::GET_ENTITY_MODEL(vehicle);
    char* makeName = "NULL";//GetVehicleMakeName(modelHash);
    string makeFinal;
    if (strcmp(UI::_GET_LABEL_TEXT(makeName), "NULL") == 0) {
        makeFinal = "";
    }
    else {
        makeFinal = string(UI::_GET_LABEL_TEXT(makeName));
    }

    return std::make_pair<string, string>(makeName, getGxtName(modelHash));
}

void update_mainmenu() {
    menu.Title("Vehicle Control");
    menu.Subtitle(string("~b~") + DISPLAY_VERSION);

    vector<string> managedVehicleNames;
    for (auto v : managedVehicles) {
        auto p = GetVehicleNames(v);
        string brand = p.first;
        string name = p.second;
        string plate = VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(v);        
        string finalName = fmt("%s (%s)", name, plate);
        managedVehicleNames.push_back(finalName);
    }

    if (managedVehicles.empty()) {
        menu.Option("No managed vehicles!");
        return;
    }
    else {
        menu.StringArray("Vehicle", managedVehicleNames, currentVehicleIndex);
    }

    Vehicle veh = managedVehicles[currentVehicleIndex];
    bool isPersistent = ENTITY::IS_ENTITY_A_MISSION_ENTITY(veh);
    bool isEngineOn = VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(veh);
    
    LockStatus lockStatus = (LockStatus)VEHICLE::GET_VEHICLE_DOOR_LOCK_STATUS(veh);
    int lockStatusIndex = distance(LockStatuses.begin(), find(LockStatuses.begin(), LockStatuses.end(), lockStatus));
    lockStatusIndex = lockStatusIndex > LockStatuses.size() ? -1 : lockStatusIndex;

    BOOL areLowBeamsOn, areHighBeamsOn;
    VEHICLE::GET_VEHICLE_LIGHTS_STATE(veh, &areLowBeamsOn, &areHighBeamsOn);
    bool areLowBeamsOn_ = areLowBeamsOn == TRUE;
    bool areHighBeamsOn_ = areHighBeamsOn == TRUE;

    bool isAlarm = VEHICLE::IS_VEHICLE_ALARM_ACTIVATED(veh);
    

    if (menu.BoolOption("Persistent", isPersistent)) {
        if (isPersistent) {
            ENTITY::SET_ENTITY_AS_MISSION_ENTITY(veh, true, false);
        }
        else {
            ENTITY::SET_ENTITY_AS_MISSION_ENTITY(veh, false, true);
            ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&veh);
        }
    }

    if (menu.BoolOption("Engine on", isEngineOn)) {
        if (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(veh)) {
            VEHICLE::SET_VEHICLE_ENGINE_ON(veh, false, true, true);
        }
        else {
            VEHICLE::SET_VEHICLE_ENGINE_ON(veh, true, true, true);
        }
    }

    int bogus = 1;
    if (menu.StringArray("Lock doors", { "", LockStatusText[lockStatus] , "" }, bogus)) {
        if (bogus == 0) {
            lockStatusIndex--;
        }
        if (bogus == 2) {
            lockStatusIndex++;
        }
        if (bogus != 1) {
            lockStatusIndex %= 2;
        }
        VEHICLE::SET_VEHICLE_DOORS_LOCKED(veh, static_cast<int>(LockStatuses[lockStatusIndex]));
    }

    if (menu.BoolOption("Low beams", areLowBeamsOn_)) {
        VEHICLE::SET_VEHICLE_LIGHTS(veh, areLowBeamsOn_ ? 3 : 4);
    }

    if (menu.BoolOption("High beams", areHighBeamsOn_)) {
        VEHICLE::SET_VEHICLE_FULLBEAM(veh, areHighBeamsOn_);
    }

    int lastDoorIndex = currentDoorIndex;
    if (menu.StringArray("Open/close doors", VehicleDoorText, currentDoorIndex)) {
        if (lastDoorIndex == currentDoorIndex) {
            if (currentDoorIndex >= NumDoors) {
                bool isAnyDoorOpen = false;
                for (int i = 0; i < NumDoors; ++i) {
                    if (VEHICLE::GET_VEHICLE_DOOR_ANGLE_RATIO(veh, i) > 0.0f) {
                        isAnyDoorOpen = true;
                        break;
                    }
                }
                if (isAnyDoorOpen) {
                    for (int i = 0; i < NumDoors; ++i) {
                        VEHICLE::SET_VEHICLE_DOOR_SHUT(veh, i, false);
                    }
                }
                else {
                    for (int i = 0; i < NumDoors; ++i) {
                        VEHICLE::SET_VEHICLE_DOOR_OPEN(veh, i, false, false);
                    }
                }
            }
            else {
                if (VEHICLE::GET_VEHICLE_DOOR_ANGLE_RATIO(veh, currentDoorIndex) > 0.0f) {
                    VEHICLE::SET_VEHICLE_DOOR_SHUT(veh, currentDoorIndex, false);
                }
                else {
                    VEHICLE::SET_VEHICLE_DOOR_OPEN(veh, currentDoorIndex, false, false);
                }
            }
        }
    }

    if (menu.BoolOption("Alarm", isAlarm)) {
        if (VEHICLE::IS_VEHICLE_ALARM_ACTIVATED(veh)) {
            VEHICLE::SET_VEHICLE_ALARM(veh, false);
        }
        else {
            VEHICLE::SET_VEHICLE_ALARM(veh, true);
            VEHICLE::START_VEHICLE_ALARM(veh);
        }
    }

    int lastHazard = currentHazard;
    if (menu.StringArray("Hazards", GenericOnOff, lastHazard)) {
        if (lastHazard == currentHazard) {
            if (lastHazard) {
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(veh, 0, true); // L
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(veh, 1, true); // R
            }
            else {
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(veh, 0, false); // L
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(veh, 1, false); // R
            }
        }
        currentHazard = lastHazard % 2;
    }
}

void update_menu() {
    menu.CheckKeys();

    if (menu.CurrentMenu("mainmenu")) {
        update_mainmenu();
    }

    menu.EndMenu();
}
