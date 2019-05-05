#include "script.h"

#include <menu.h>
#include <inc/natives.h>

#include "Util/Paths.h"
#include "Util/Logger.hpp"
#include "Util/UIUtils.h"
#include "Versions.h"
#include "ManagedVehicle.h"
#include "Util/StringFormat.h"

// TODO: Control trailer
// TODO: Get in as passenger

bool CanUseNeonNative = false;

NativeMenu::Menu menu;
std::string settingsMenuFile;

Player player = 0;
Ped playerPed = 0;
Vehicle currentVehicle = 0;
Vehicle lastVehicle = 0;

std::vector<ManagedVehicle> managedVehicles;

// Externally set when init radio stations is done
// Inits to "OFF" index in the script radio list
int OffStationIndex = 0;

// Implementation in ScriptMenu.cpp but too lazy to properly move around atm
std::string getGxtName(Hash hash);


// bah
extern std::vector<std::string> RadioStationNames;
int getFixedRadioIdx() {
    if (AUDIO::GET_PLAYER_RADIO_STATION_INDEX() > RadioStationNames.size() - 1)
        return RadioStationNames.size() - 1;
    return AUDIO::GET_PLAYER_RADIO_STATION_INDEX();
}

void update_game() {
    player = PLAYER::PLAYER_ID();
    playerPed = PLAYER::PLAYER_PED_ID();
    currentVehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, true);

    if (!ENTITY::DOES_ENTITY_EXIST(currentVehicle))
        return;

    auto foundVehicle = std::find_if(managedVehicles.begin(), managedVehicles.end(), 
        [](ManagedVehicle const& v) { return v.Vehicle == currentVehicle; });
    if (foundVehicle == managedVehicles.end()) {
        managedVehicles.emplace_back(currentVehicle, getFixedRadioIdx());
        std::string blipName = fmt("%s (%s)", 
            getGxtName(ENTITY::GET_ENTITY_MODEL(currentVehicle)), 
            VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(currentVehicle));
        managedVehicles.back().Blip = new Blip_(currentVehicle, BlipSpriteStandard, blipName, BlipColorWhite, true);
        managedVehicles.back().Blip->SetAlpha(0);
    }
}

void update_managedVehicles() {
    std::vector<ManagedVehicle> stale;
    for (auto &v : managedVehicles) {
        if (!ENTITY::DOES_ENTITY_EXIST(v.Vehicle)) {
            stale.push_back(v);
        }
        else {
            if (PED::IS_PED_IN_VEHICLE(playerPed, v.Vehicle, false))
                v.RadioIndex = getFixedRadioIdx();
        }
    }

    for (auto v : stale) {
        v.Blip->Delete();
        delete v.Blip;
        managedVehicles.erase(std::remove(managedVehicles.begin(), managedVehicles.end(), v), managedVehicles.end());
    }
}

void main() {
    CanUseNeonNative = getGameVersion() >= G_VER_1_0_573_1_STEAM;
	AUDIO::SET_AUDIO_FLAG("LoadMPData", true);
    logger.Write(INFO, "Script started");

    settingsMenuFile = Paths::GetModuleFolder(Paths::GetOurModuleHandle()) + modDir + "\\settings_menu.ini";
    menu.SetFiles(settingsMenuFile);
    logger.Write(INFO, "Loading " + settingsMenuFile);
    menu.ReadSettings();

    menu.RegisterOnMain(std::bind(onMain));
    menu.RegisterOnExit(std::bind(onExit));

    initRadioStations();

    while (true) {
        update_game();
        update_managedVehicles();
        update_menu();
        WAIT(0);
    }
}

void ScriptMain() {
    srand(GetTickCount());
    main();
}
