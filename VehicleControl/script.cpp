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

std::vector<Vehicle> managedVehicles;

void update_game() {
    player = PLAYER::PLAYER_ID();
    playerPed = PLAYER::PLAYER_PED_ID();
    currentVehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, true);

    if (!ENTITY::DOES_ENTITY_EXIST(currentVehicle))
        return;

    auto foundVehicle = std::find(managedVehicles.begin(), managedVehicles.end(), currentVehicle);
    if (foundVehicle == managedVehicles.end()) {
        managedVehicles.push_back(currentVehicle);
    }
}

void update_managedVehicles() {
    std::vector<Vehicle> stale;
    for (auto v : managedVehicles) {
        if (!ENTITY::DOES_ENTITY_EXIST(v))
            stale.push_back(v);
    }

    for (auto v : stale) {
        managedVehicles.erase(std::remove(managedVehicles.begin(), managedVehicles.end(), v), managedVehicles.end());
    }
}

void main() {
    logger.Write(INFO, "Script started");

    settingsMenuFile = Paths::GetModuleFolder(Paths::GetOurModuleHandle()) + modDir + "\\settings_menu.ini";
    menu.SetFiles(settingsMenuFile);
    logger.Write(INFO, "Loading " + settingsMenuFile);
    menu.ReadSettings();

    menu.RegisterOnMain(std::bind(onMain));
    menu.RegisterOnExit(std::bind(onExit));

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
