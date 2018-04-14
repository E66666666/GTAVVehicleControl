#include "script.h"

#include <menu.h>
#include "Util/StringFormat.h"
#include "Util/UIUtils.h"
#include "Util/Logger.hpp"

using namespace std;

extern NativeMenu::Menu menu;

extern Player player;
extern Ped playerPed;
extern Vehicle currentVehicle;
extern Vehicle lastVehicle;

extern vector<ManagedVehicle> managedVehicles;

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

vector<Door> Doors {
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

vector<string> VehicleDoorBones{
    "door_dside_f",
    "door_pside_f",
    "door_dside_r",
    "door_pside_r",
    "bonnet",
    "boot",
};

enum class BombBayAction {
    Open,
    Close
};

vector<BombBayAction> BombBayStates{
    BombBayAction::Open,
    BombBayAction::Close
};

vector<string> BombBayText {
    "Open",
    "Close"
};

enum class Blinker {
    Off,
    Left,
    Right,
    Hazard
};

vector<Blinker> Blinkers {
    Blinker::Off,
    Blinker::Left,
    Blinker::Right,
    Blinker::Hazard
};

vector<string> BlinkerText {
    "Off",
    "Left",
    "Right",
    "Hazard"
};

// Nice name, station number, internal name
unordered_map<string, pair<int, string>> RadioStations;
// Nice names!
vector<string> RadioStationNames;

int currentVehicleIndex = 0;

//int currentDoorIndex = 0;
//int currentHazard = 0;

void onMain() {
    menu.ReadSettings();
}

void onExit() {

}

void initRadioStations() {
    for (int i = 0; i < 256; i++) {
        if (strcmp(UI::_GET_LABEL_TEXT(AUDIO::GET_RADIO_STATION_NAME(i)), "NULL") != 0) {
            RadioStations.try_emplace(string(UI::_GET_LABEL_TEXT(AUDIO::GET_RADIO_STATION_NAME(i))), make_pair(i, string(AUDIO::GET_RADIO_STATION_NAME(i)) ));
            RadioStationNames.push_back(UI::_GET_LABEL_TEXT(AUDIO::GET_RADIO_STATION_NAME(i)));
        }
    }
    RadioStations.try_emplace("Radio Off", make_pair( 255, "OFF" ));
    RadioStationNames.push_back("Radio Off");

    for (auto it = RadioStations.begin(); it != RadioStations.end(); ++it) {
        logger.Write(INFO, "%d\t%s %s", it->second.first, it->second.second.c_str(), it->first.c_str());
    }
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

bool HasBone(Entity e, char* bone) {
    return ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(e, bone) != -1;
}

bool HasDoors(Entity e) {
    for (auto b: VehicleDoorBones) {
        if (HasBone(e, (char*)b.c_str())) {
            return true;
        }
    }
    return false;
}

void update_mainmenu() {
    menu.Title("Vehicle Control");
    menu.Subtitle(string("~b~") + DISPLAY_VERSION);

    if (managedVehicles.empty()) {
        menu.Option("No managed vehicles!");
        return;
    }

    if (currentVehicleIndex >= managedVehicles.size())
        currentVehicleIndex = managedVehicles.size() - 1;

    vector<string> managedVehicleNames;
    for (auto v : managedVehicles) {
        auto p = GetVehicleNames(v.Vehicle);
        string brand = p.first;
        string name = p.second;
        string plate = VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(v.Vehicle);        
        string finalName = fmt("%s (%s)", name, plate);
        managedVehicleNames.push_back(finalName);
    }

    ManagedVehicle& mVeh = managedVehicles[currentVehicleIndex];
    Vehicle veh = mVeh.Vehicle;
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

    menu.StringArray("Vehicle", managedVehicleNames, currentVehicleIndex);

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

    int lastRadio = mVeh.RadioIndex;
    if (menu.StringArray("Radio", RadioStationNames, mVeh.RadioIndex)) {
        if (lastRadio == mVeh.RadioIndex) {
            AUDIO::SET_VEHICLE_RADIO_ENABLED(veh, true);
            AUDIO::SET_VEHICLE_RADIO_LOUD(veh, true);
            AUDIO::_0xC1805D05E6D4FE10(veh);
            AUDIO::SET_VEH_RADIO_STATION(veh, (char *)RadioStations[RadioStationNames[mVeh.RadioIndex]].second.c_str());
        }
    }

    if (menu.BoolOption("Lights", areLowBeamsOn_)) {
        VEHICLE::SET_VEHICLE_LIGHTS(veh, areLowBeamsOn_ ? 3 : 4);
    }

    if (areLowBeamsOn_) {
        if (menu.BoolOption("High beams", areHighBeamsOn_)) {
            VEHICLE::SET_VEHICLE_FULLBEAM(veh, areHighBeamsOn_);
        }
    }

    int lastBlinker = mVeh.BlinkerIndex;
    if (menu.StringArray("Indicators", BlinkerText, lastBlinker)) {
        if (lastBlinker == mVeh.BlinkerIndex) {
            switch ((Blinker)lastBlinker) {
            case Blinker::Left: {
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(veh, 0, false); // L
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(veh, 1, true); // R
                break;
            }
            case Blinker::Right: {
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(veh, 0, true); // L
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(veh, 1, false); // R
                break;
            }
            case Blinker::Hazard: {
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(veh, 0, true); // L
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(veh, 1, true); // R
                break;
            }
            default: {
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(veh, 0, false); // L
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(veh, 1, false); // R
                break;
            }
            }
        }
        mVeh.BlinkerIndex = lastBlinker % BlinkerText.size();
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

    if (VEHICLE::IS_VEHICLE_A_CONVERTIBLE(veh, false)) {
        if (menu.Option("Toggle roof")) {
            if (VEHICLE::GET_CONVERTIBLE_ROOF_STATE(veh) == 0) {
                VEHICLE::LOWER_CONVERTIBLE_ROOF(veh, false);
            }
            if (VEHICLE::GET_CONVERTIBLE_ROOF_STATE(veh) == 2) {
                VEHICLE::RAISE_CONVERTIBLE_ROOF(veh, false);
            }
        }
    }
    
    if (HasBone(veh, "door_hatch_l") && HasBone(veh, "door_hatch_r")) {
        if (menu.Option("Toggle bomb bays")) {
            mVeh.BombBayIndex++;
            mVeh.BombBayIndex %= 2; 
            if (mVeh.BombBayIndex == 0) {
                VEHICLE::CLOSE_BOMB_BAY_DOORS(veh);
            }
            if (mVeh.BombBayIndex == 1) {
                VEHICLE::OPEN_BOMB_BAY_DOORS(veh);
            }
        }
    }

    if (HasDoors(veh)) {
        
        int bogus = 1;
        if (menu.StringArray("Lock doors", { "", LockStatusText[lockStatus] , "" }, bogus)) {
            if (bogus != 1) {
                lockStatusIndex++;
            }
            lockStatusIndex = lockStatusIndex % 2;
            VEHICLE::SET_VEHICLE_DOORS_LOCKED(veh, static_cast<int>(LockStatuses[lockStatusIndex]));
        }

        int lastDoorIndex = mVeh.DoorIndex;
        if (menu.StringArray("Open/close doors", VehicleDoorText, mVeh.DoorIndex)) {
            if (lastDoorIndex == mVeh.DoorIndex) {
                if (mVeh.DoorIndex >= NumDoors) {
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
                    if (VEHICLE::GET_VEHICLE_DOOR_ANGLE_RATIO(veh, mVeh.DoorIndex) > 0.0f) {
                        VEHICLE::SET_VEHICLE_DOOR_SHUT(veh, mVeh.DoorIndex, false);
                    }
                    else {
                        VEHICLE::SET_VEHICLE_DOOR_OPEN(veh, mVeh.DoorIndex, false, false);
                    }
                }
            }
            else {
                if (mVeh.DoorIndex < NumDoors) {
                    if (mVeh.DoorIndex > lastDoorIndex) {
                        while (!HasBone(veh, (char*)VehicleDoorBones[mVeh.DoorIndex].c_str())) {
                            mVeh.DoorIndex++;
                            if (mVeh.DoorIndex >= NumDoors) {
                                mVeh.DoorIndex = 0;
                            }
                        }
                    }
                    else if (mVeh.DoorIndex < lastDoorIndex) {
                        while (!HasBone(veh, (char*)VehicleDoorBones[mVeh.DoorIndex].c_str())) {
                            mVeh.DoorIndex--;
                            if (mVeh.DoorIndex < 0) {
                                mVeh.DoorIndex = NumDoors - 1;
                            }
                        }
                    }
                }
            }
        }
    }
}

void update_menu() {
    menu.CheckKeys();

    if (menu.CurrentMenu("mainmenu")) {
        update_mainmenu();
    }

    menu.EndMenu();
}
