#include <inc\main.h>

#include "script.h"
#include "Util/Paths.h"
#include "Util/Logger.hpp"
#include "Versions.h"

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved) {
    std::string logFile = Paths::GetModuleFolder(hInstance) + modDir +
        "\\" + Paths::GetModuleNameWithoutExtension(hInstance) + ".log";
    logger.SetFile(logFile);
    Paths::SetOurModuleHandle(hInstance);

    switch (reason) {
        case DLL_PROCESS_ATTACH:
            scriptRegister(hInstance, ScriptMain);
            logger.Clear();
            logger.Write(INFO, "GTAVVehicleControl " + std::string(DISPLAY_VERSION));
            logger.Write(INFO, "Game version " + eGameVersionToString(getGameVersion()));
            break;
        case DLL_PROCESS_DETACH:
            scriptUnregister(hInstance);
            break;
        default:
            break;
    }
    return TRUE;
}
