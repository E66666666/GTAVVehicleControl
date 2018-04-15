#include "script.h"

#include <menu.h>
#include "Util/StringFormat.h"
#include "Util/UIUtils.h"

using namespace std;

const int NumDoors = 6;
const int NumWindows = 4;

extern NativeMenu::Menu menu;

extern Player player;
extern Ped playerPed;
extern Vehicle currentVehicle;
extern Vehicle lastVehicle;

extern vector<ManagedVehicle> managedVehicles;
extern int OffStationIndex;

int currentVehicleIndex = 0;
bool pendingExtern = false;
vector<function<void(void)>> pendingTaskSequence;

Object fob = 0;
bool fobPlaying = false;
bool fobBeepOn = false;
bool fobBlop = false;

// GXT name, station number, internal name
unordered_map<string, pair<int, string>> RadioStations;
vector<string> RadioStationNames; // GXT names

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
    "vc_all"
};

vector<string> VehicleWindowText {
    "Front Right",
    "Front Left",
    "Rear Right",
    "Rear Left",
    "All Windows"
};

vector<string> VehicleWindowBones {
    "window_rf",
    "window_lf",
    "window_rr",
    "window_lr",
    "vc_all"
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

vector<eVehicleNeonLight> Neons {
    VehicleNeonLightLeft,
    VehicleNeonLightRight,
    VehicleNeonLightFront,
    VehicleNeonLightBack,
};

vector<string> NeonText {
    "Left",
    "Right",
    "Front",
    "Back",
    "All"
};

unordered_map<eVehicleNeonLight, string> NeonBonesMap {
    { VehicleNeonLightLeft,  "neon_l" },
    { VehicleNeonLightRight, "neon_r" },
    { VehicleNeonLightFront, "neon_f" },
    { VehicleNeonLightBack,  "neon_b" },
};

vector<string> SirenBones {
    "siren1" ,
    "siren2" ,
    "siren3" ,
    "siren4" ,
    "siren5" ,
    "siren6" ,
    "siren7" ,
    "siren8" ,
    "siren9" ,
    "siren10",
    "siren11",
    "siren12",
    "siren13",
    "siren14",
    "siren15",
    "siren16",
    "siren17",
    "siren18",
    "siren19",
    "siren20",
};

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
    OffStationIndex = RadioStationNames.size() - 1;
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
    if (strcmp(bone, "vc_all") == 0) 
        return true;
    return ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(e, bone) != -1;
}

bool HasDoors(Entity e) {
    for (auto b: VehicleDoorBones) {
        if (b == "vc_all") continue;
        if (HasBone(e, (char*)b.c_str())) {
            return true;
        }
    }
    return false;
}

void executePendingSequence() {
    for (auto task : pendingTaskSequence) {
        task();
    }
    pendingTaskSequence.clear();
}

void PlayFobAnim(bool beepOn) {
    if (PED::IS_PED_IN_ANY_VEHICLE(PLAYER::PLAYER_PED_ID(), false)) {
        executePendingSequence();
        pendingExtern = false;
        return;
    }
    pendingExtern = true;
    fobBeepOn = beepOn;

    auto coords =  ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), 0);
    fob = OBJECT::CREATE_OBJECT(GAMEPLAY::GET_HASH_KEY("lr_prop_carkey_fob"), coords.x, coords.y, coords.z, true, false, false);
    ENTITY::ATTACH_ENTITY_TO_ENTITY(fob, PLAYER::PLAYER_PED_ID(), PED::GET_PED_BONE_INDEX(PLAYER::PLAYER_PED_ID(), 28422), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1, 0, 0, 0, 2, 1);

    STREAMING::REQUEST_ANIM_DICT("anim@mp_player_intmenu@key_fob@");
    while (!STREAMING::HAS_ANIM_DICT_LOADED("anim@mp_player_intmenu@key_fob@")) {
        WAIT(0);
    }
    WEAPON::SET_CURRENT_PED_WEAPON(PLAYER::PLAYER_PED_ID(), GAMEPLAY::GET_HASH_KEY("weapon_unarmed"), false);
    AI::TASK_PLAY_ANIM(PLAYER::PLAYER_PED_ID(), "anim@mp_player_intmenu@key_fob@", "fob_click", 8.0f, -8.0f, -1, 16 | 32, 0, 0, 0, 0);
}

void UpdateFob() {
    if (ENTITY::IS_ENTITY_PLAYING_ANIM(PLAYER::PLAYER_PED_ID(), "anim@mp_player_intmenu@key_fob@", "fob_click", 3)) {
        fobPlaying = true;
    }

    if (fobPlaying && !fobBlop) {
        if (ENTITY::GET_ENTITY_ANIM_CURRENT_TIME(PLAYER::PLAYER_PED_ID(), "anim@mp_player_intmenu@key_fob@", "fob_click") >= 0.226) {
            AUDIO::PLAY_SOUND_FROM_ENTITY(-1, "Remote_Control_Fob", PLAYER::PLAYER_PED_ID(), "PI_Menu_Sounds", 1, 0);
            if (fobBeepOn) {
                AUDIO::PLAY_SOUND_FROM_ENTITY(-1, "Remote_Control_Open", managedVehicles[currentVehicleIndex].Vehicle, "PI_Menu_Sounds", 1, 0);
            }
            else {
                AUDIO::PLAY_SOUND_FROM_ENTITY(-1, "Remote_Control_Close", managedVehicles[currentVehicleIndex].Vehicle, "PI_Menu_Sounds", 1, 0);
            }
            fobBlop = true;
            pendingExtern = false;
            executePendingSequence();
        }
    }

    if (fob != 0 && fobPlaying) {
        if (ENTITY::IS_ENTITY_PLAYING_ANIM(PLAYER::PLAYER_PED_ID(), "anim@mp_player_intmenu@key_fob@", "fob_click", 3)) {
            return;
        }
        OBJECT::DELETE_OBJECT(&fob);
        fob = 0;
        fobPlaying = false;
        fobBlop = false;
    }
}

bool HasNeon(Vehicle veh, eVehicleNeonLight neon) {
    return HasBone(veh, (char*)NeonBonesMap[neon].c_str());
}

bool HasNeon(Vehicle veh) {
    bool hasNeon = false;
    for (auto neon : NeonBonesMap) {
        if (HasNeon(veh, neon.first)) {
            hasNeon = true;
            break;
        }
    }
    return hasNeon;
}

bool HasSiren(Vehicle veh) {
    for (auto siren : SirenBones) {
        if (HasBone(veh, (char*)siren.c_str()))
            return true;
    }
    return false;
}

string OptionNoVehicles = "No managed vehicles!";
vector<string> OptionNoVehiclesDescription = { "Vehicle Control keeps track of vehicles you entered. Enter a vehicle to use the script!" };

string OptionVehicle = "Vehicle";
string OptionVehicleDescriptionPart = "Currently controlled vehicle." ;

string OptionPersistent = "Persistent";
vector<string> OptionPersistentDescription = { "Prevent the vehicle from being removed by the game." };

string OptionEngineOn = "Engine on";
vector<string> OptionEngineOnDescription = { "Pre-warm the car for those cold winter days." };

string OptionRadio = "Radio";
vector<string> OptionRadioDescription = { "Did someone say hearing loss?" };

string OptionLights = "Lights";
vector<string> OptionLightsDescription = { "Honey did you forget to turn the lights off again?" };

string OptionFullbeam = "Full beam";
vector<string> OptionFullbeamDescription = { "Blind yourself remotely!" };

string OptionBlinkers = "Indicators";
vector<string> OptionBlinkersDescription = { "More blinkenlights are always better." };

string OptionAlarm = "Alarm";
vector<string> OptionAlarmDescription = { "For if you need to scare your neighbors' kids from your car." };

string OptionRoof = "Toggle roof";
vector<string> OptionRoofDescription = { "Your convertible is parked outside, it's starting to rain, but you can't get up." };

string OptionBombbay = "Toggle bomb bays";
vector<string> OptionBombbayDescription = { "Oh, that's why she was flying like a brick!" };

string OptionLock = "Lock doors";
vector<string> OptionLockDescription = { "It would suck having your car stolen!" };

string OptionDoors = "Open/close doors";
vector<string> OptionDoorsDescription = { "Steal the show at a car meet." };

string OptionWindows = "Roll windows up/down";
vector<string> OptionWindowsDescription = { "Did you really think your beater would have A/C?" };

string OptionNeon = "Neons";
vector<string> OptionNeonDescription = { "<something snarky>" };

string OptionSiren = "Sirens";
vector<string> OptionSirenDescription = { "I AM THE LAW!" };

void update_mainmenu() {
    menu.Title("Vehicle Control");
    menu.Subtitle(string("~b~") + DISPLAY_VERSION);

    if (managedVehicles.empty()) {
        menu.Option(OptionNoVehicles, OptionNoVehiclesDescription);
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
    bool isSiren = VEHICLE::IS_VEHICLE_SIREN_ON(veh);

    vector<string> OptionVehicleDescription = { 
        OptionVehicleDescriptionPart, 
        fmt("%d / %d", currentVehicleIndex + 1, managedVehicleNames.size()) 
    };
    menu.StringArray(OptionVehicle, managedVehicleNames, currentVehicleIndex, OptionVehicleDescription);

    if (menu.BoolOption(OptionPersistent, isPersistent, OptionPersistentDescription)) {
        if (isPersistent) {
            ENTITY::SET_ENTITY_AS_MISSION_ENTITY(veh, true, false);
        }
        else {
            ENTITY::SET_ENTITY_AS_MISSION_ENTITY(veh, false, true);
            ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&veh);
        }
    }

    if (menu.BoolOption(OptionEngineOn, isEngineOn, OptionEngineOnDescription)) {
        if (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(veh)) {
            PlayFobAnim(false);
            pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_ENGINE_ON, veh, false, true, true));
        }
        else {
            PlayFobAnim(true);
            pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_ENGINE_ON, veh, true, true, true));
        }
    }

    int lastRadio = mVeh.RadioIndex;
    if (menu.StringArray(OptionRadio, RadioStationNames, mVeh.RadioIndex, OptionRadioDescription)) {
        if (lastRadio == mVeh.RadioIndex) {
            if (RadioStationNames[mVeh.RadioIndex] == "Radio off") {
                PlayFobAnim(false);
            }
            else {
                PlayFobAnim(true);
            }
            pendingTaskSequence.push_back(bind(&AUDIO::SET_VEHICLE_RADIO_ENABLED, veh, true));
            pendingTaskSequence.push_back(bind(&AUDIO::SET_VEHICLE_RADIO_LOUD,veh, true));
            pendingTaskSequence.push_back(bind(&AUDIO::_0xC1805D05E6D4FE10,veh));
            pendingTaskSequence.push_back(bind(&AUDIO::SET_VEH_RADIO_STATION,veh, (char *)RadioStations[RadioStationNames[mVeh.RadioIndex]].second.c_str()));
        }
    }

    if (menu.BoolOption(OptionLights, areLowBeamsOn_, OptionLightsDescription)) {
        PlayFobAnim(areLowBeamsOn_);
        pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_LIGHTS, veh, areLowBeamsOn_ ? 3 : 4));
    }

    if (areLowBeamsOn_) {
        if (menu.BoolOption(OptionFullbeam, areHighBeamsOn_, OptionFullbeamDescription)) {
            PlayFobAnim(areHighBeamsOn_);
            pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_FULLBEAM, veh, areHighBeamsOn_));
        }
    }

    int lastBlinker = mVeh.BlinkerIndex;
    if (menu.StringArray(OptionBlinkers, BlinkerText, lastBlinker, OptionBlinkersDescription)) {
        if (lastBlinker == mVeh.BlinkerIndex) {
            switch ((Blinker)lastBlinker) {
            case Blinker::Left: {
                PlayFobAnim(true);
                pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS, veh, 0, false)); // L
                pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS, veh, 1, true)); // R
                break;                                                                  
            }                                                                           
            case Blinker::Right: {                                                      
                PlayFobAnim(true);                                                      
                pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS, veh, 0, true)); // L
                pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS, veh, 1, false)); // R
                break;                                                                   
            }                                                                            
            case Blinker::Hazard: {                                                      
                PlayFobAnim(true);                                                       
                pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS, veh, 0, true)); // L
                pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS, veh, 1, true)); // R
                break;                                                                  
            }                                                                           
            default: {                                                                  
                PlayFobAnim(false);                                                     
                pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS, veh, 0, false)); // L
                pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS, veh, 1, false)); // R
                break;
            }
            }
        }
        mVeh.BlinkerIndex = lastBlinker % BlinkerText.size();
    }

    if (menu.BoolOption(OptionAlarm, isAlarm, OptionAlarmDescription)) {
        if (VEHICLE::IS_VEHICLE_ALARM_ACTIVATED(veh)) {
            PlayFobAnim(false);
            pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_ALARM, veh, false));
        }
        else {
            PlayFobAnim(true);
            pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_ALARM, veh, true));
            pendingTaskSequence.push_back(bind(&VEHICLE::START_VEHICLE_ALARM, veh));
        }
    }

    if (HasSiren(veh)) {
        if (menu.BoolOption(OptionSiren, isSiren, OptionSirenDescription)) {
            bool isSirenOn = VEHICLE::IS_VEHICLE_SIREN_ON(veh);
            VEHICLE::SET_VEHICLE_SIREN(veh, !isSirenOn);
        }
    }

    if (HasNeon(veh)) {
        int lastNeonIndex = mVeh.NeonIndex;
        if (menu.StringArray(OptionNeon, NeonText, mVeh.NeonIndex, OptionNeonDescription)) {            
            bool isAnyNeonOn = false;
            for (int i = 0; i < 4; ++i) {
                isAnyNeonOn |= VEHICLE::_IS_VEHICLE_NEON_LIGHT_ENABLED(veh, i) == TRUE;
            }
            if (lastNeonIndex == mVeh.NeonIndex) {
                if (NeonText[lastNeonIndex] == "All") {
                    PlayFobAnim(!isAnyNeonOn);
                    for (int i = 0; i < 4; ++i) {
                        VEHICLE::_SET_VEHICLE_NEON_LIGHT_ENABLED(veh, i, !isAnyNeonOn);
                    }
                }
                else {
                    PlayFobAnim(!VEHICLE::_IS_VEHICLE_NEON_LIGHT_ENABLED(veh, lastNeonIndex));
                    VEHICLE::_SET_VEHICLE_NEON_LIGHT_ENABLED(veh, lastNeonIndex, !VEHICLE::_IS_VEHICLE_NEON_LIGHT_ENABLED(veh, lastNeonIndex));
                }
            }
        }
    }

    if (VEHICLE::IS_VEHICLE_A_CONVERTIBLE(veh, false)) {
        if (menu.Option(OptionRoof, OptionRoofDescription)) {
            if (VEHICLE::GET_CONVERTIBLE_ROOF_STATE(veh) == 0) {
                PlayFobAnim(true);
                pendingTaskSequence.push_back(bind(&VEHICLE::LOWER_CONVERTIBLE_ROOF, veh, false));
            }
            if (VEHICLE::GET_CONVERTIBLE_ROOF_STATE(veh) == 2) {
                PlayFobAnim(false);
                pendingTaskSequence.push_back(bind(&VEHICLE::RAISE_CONVERTIBLE_ROOF, veh, false));
            }
        }
    }
    
    if (HasBone(veh, "door_hatch_l") && HasBone(veh, "door_hatch_r")) {
        if (menu.Option(OptionBombbay, OptionBombbayDescription)) {
            mVeh.BombBayIndex++;
            mVeh.BombBayIndex %= 2; 
            if (mVeh.BombBayIndex == 0) {
                PlayFobAnim(false);
                pendingTaskSequence.push_back(bind(&VEHICLE::CLOSE_BOMB_BAY_DOORS, veh));
            }
            if (mVeh.BombBayIndex == 1) {
                PlayFobAnim(true);
                pendingTaskSequence.push_back(bind(&VEHICLE::OPEN_BOMB_BAY_DOORS, veh));
            }
        }
    }

    if (HasDoors(veh)) {
        
        int bogus = 1;
        if (menu.StringArray(OptionLock, { "", LockStatusText[lockStatus] , "" }, bogus, OptionLockDescription)) {
            PlayFobAnim(true);
            if (bogus != 1) {
                lockStatusIndex++;
            }
            lockStatusIndex = lockStatusIndex % 2;
            pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_DOORS_LOCKED, veh, static_cast<int>(LockStatuses[lockStatusIndex])));
        }

        int lastDoorIndex = mVeh.DoorIndex;
        if (menu.StringArray(OptionDoors, VehicleDoorText, mVeh.DoorIndex, OptionDoorsDescription)) {
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
                            PlayFobAnim(false);
                            pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_DOOR_SHUT, veh, i, false));
                        }
                    }
                    else {
                        for (int i = 0; i < NumDoors; ++i) {
                            PlayFobAnim(true);
                            pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_DOOR_OPEN, veh, i, false, false));
                        }
                    }
                }
                else {
                    if (VEHICLE::GET_VEHICLE_DOOR_ANGLE_RATIO(veh, mVeh.DoorIndex) > 0.0f) {
                        PlayFobAnim(false);
                        pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_DOOR_SHUT, veh, mVeh.DoorIndex, false));
                    }
                    else {
                        PlayFobAnim(true);
                        pendingTaskSequence.push_back(bind(&VEHICLE::SET_VEHICLE_DOOR_OPEN, veh, mVeh.DoorIndex, false, false));
                    }
                }
            }
            else {
                if (mVeh.DoorIndex < VehicleDoorBones.size()) {
                    if (mVeh.DoorIndex > lastDoorIndex) {
                        while (!HasBone(veh, (char*)VehicleDoorBones[mVeh.DoorIndex].c_str())) {
                            mVeh.DoorIndex++;
                            if (mVeh.DoorIndex >= VehicleDoorBones.size()) {
                                mVeh.DoorIndex = 0;
                            }
                        }
                    }
                    else if (mVeh.DoorIndex < lastDoorIndex) {
                        while (!HasBone(veh, (char*)VehicleDoorBones[mVeh.DoorIndex].c_str())) {
                            mVeh.DoorIndex--;
                            if (mVeh.DoorIndex < 0) {
                                mVeh.DoorIndex = VehicleDoorBones.size() - 1;
                            }
                        }
                    }
                }
            }
        }

        //int lastWindowIndex = mVeh.WindowIndex;
        //if (menu.StringArray(OptionWindows, VehicleWindowText, mVeh.WindowIndex, OptionWindowsDescription)) {
        //    if (lastWindowIndex == mVeh.WindowIndex) {
        //        if (mVeh.WindowIndex >= NumWindows) {
        //            bool isAnyWindowDown = false;
        //            for (int i = 0; i < NumWindows; ++i) {
        //                VEHICLE::WINDOW
        //            }
        //        }
        //        else {
        //            
        //        }
        //    }
        //}
    }
}

void update_menu() {
    menu.CheckKeys();

    if (menu.CurrentMenu("mainmenu")) {
        update_mainmenu();
    }

    menu.EndMenu();
    UpdateFob();
}
