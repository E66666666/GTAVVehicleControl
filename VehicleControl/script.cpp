#include "script.h"

#include <menu.h>

#include "Util/Paths.h"
#include "Util/Logger.hpp"
#include "Util/UIUtils.h"

int textureBgId;
int textureBgId2;

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

void update_game() {
    player = PLAYER::PLAYER_ID();
    playerPed = PLAYER::PLAYER_PED_ID();
    currentVehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, true);

    if (!ENTITY::DOES_ENTITY_EXIST(currentVehicle))
        return;

    auto foundVehicle = std::find_if(managedVehicles.begin(), managedVehicles.end(), 
        [](ManagedVehicle const& v) { return v.Vehicle == currentVehicle; });
    if (foundVehicle == managedVehicles.end()) {
        // TODO: Something to determine current vehicle radio station.
        managedVehicles.push_back(ManagedVehicle(currentVehicle, OffStationIndex));
    }
}

void update_managedVehicles() {
    std::vector<ManagedVehicle> stale;
    for (auto v : managedVehicles) {
        if (!ENTITY::DOES_ENTITY_EXIST(v.Vehicle))
            stale.push_back(v);
    }

    for (auto v : stale) {
        managedVehicles.erase(std::remove(managedVehicles.begin(), managedVehicles.end(), v), managedVehicles.end());
    }
}

void main() {
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
