#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <iostream>
#include <MinHook.h>


#if _WIN64 
#pragma comment(lib, "libMinHook.x64.lib")
#else
#pragma comment(lib, "libMinHook.x86.lib")
#endif


//Globals
HINSTANCE DllHandle;


// hooks
typedef BOOL(WINAPI* peekMessageA)(LPMSG lpMsg, HWND  hWnd, UINT  wMsgFilterMin, UINT  wMsgFilterMax, UINT  wRemoveMsg);
peekMessageA pPeekMessageA = nullptr; //original function pointer after hook
peekMessageA pPeekMessageATarget; //original function pointer BEFORE hook do not call this!
BOOL WINAPI detourPeekMessageA(LPMSG lpMsg, HWND  hWnd, UINT  wMsgFilterMin, UINT  wMsgFilterMax, UINT  wRemoveMsg) {
    if (lpMsg->message == WM_KEYDOWN) {
        std::cout << "Key pressed" << std::endl;
    }
    return pPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
}

typedef void (__cdecl* reload)(unsigned int a);
reload pReload = nullptr;
reload pReloadTarget = reinterpret_cast<reload>(0x004C3900); // function pointer before hook
void __cdecl detourReload(unsigned int enitylistIndex) {

    int weaponIndex; //weaponIndex
    __asm mov weaponIndex, ecx;

    std::cout << "Enitylist Index: " << (enitylistIndex & 0xffff) << std::endl; //weapon entity index?!
    std::cout << "Weapon index: " << weaponIndex << std::endl;

    __asm mov ecx, weaponIndex;

    return pReload(enitylistIndex++);
}

DWORD __stdcall EjectThread(LPVOID lpParameter) {
    Sleep(100);
    FreeLibraryAndExitThread(DllHandle, 0);
    return 0;
}

void shutdown(FILE* fp, std::string reason) {

    MH_Uninitialize();
    std::cout << reason << std::endl;
    Sleep(1000);
    if (fp != nullptr)
        fclose(fp);
    FreeConsole();
    CreateThread(0, 0, EjectThread, 0, 0, 0);
    return;
}

DWORD WINAPI Menue(HINSTANCE hModule) {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout); //sets cout to be used with our newly created console

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK)
    {
        std::string sStatus = MH_StatusToString(status);
        shutdown(fp, "Minhook init failed!");
        return 0;
    }

    if (MH_CreateHookApiEx(L"user32", "PeekMessageA", &detourPeekMessageA, reinterpret_cast<void**>(&pPeekMessageA), reinterpret_cast<void**>(&pPeekMessageATarget)) != MH_OK) {
        shutdown(fp, "PeekMessageA CreateHook failed!");
        return 1;
    }

    if (MH_CreateHook(reinterpret_cast<void**>(pReloadTarget), &detourReload, reinterpret_cast<void**>(&pReload)) != MH_OK) {
        shutdown(fp, "CreateHook failed!");
        return 1;
    }

    std::cout << "[0] Exit" << std::endl;
    std::cout << "[1] PeekMessage" << std::endl;
    std::cout << "[2] Reload" << std::endl;

    bool enablePeekMessage = false;
    bool enableReload = false;
    while (true) {
        Sleep(50);
        if (GetAsyncKeyState(VK_NUMPAD0) & 1) {
            break;
        }
        if (GetAsyncKeyState(VK_NUMPAD1) & 1) {
            enablePeekMessage = !enablePeekMessage;
            if (enablePeekMessage) {
                std::cout << "PeekMessage enabled" << std::endl;
                if (MH_EnableHook(pPeekMessageATarget) != MH_OK) {
                    shutdown(fp, "PeekMessage failed!");
                    return 1;
                }
            }
            else {
                std::cout << "PeekMessage disabled" << std::endl;
                if (MH_DisableHook(pPeekMessageATarget) != MH_OK) {
                    shutdown(fp, "DisableHook failed!");
                    return 1;
                }
            }
        }
        if (GetAsyncKeyState(VK_NUMPAD2) & 1) {
            enableReload = !enableReload;
            if (enableReload) {
                std::cout << "Reload enabled" << std::endl;
                if (MH_EnableHook(reinterpret_cast<void**>(pReloadTarget)) != MH_OK) {
                    shutdown(fp, "Reload: EnableHook failed!");
                    return 1;
                }
            }
            else {
                std::cout << "Reload disabled" << std::endl;
                if (MH_DisableHook(reinterpret_cast<void**>(pReloadTarget)) != MH_OK) {
                    shutdown(fp, "Reload: DisableHook failed!");
                    return 1;
                }
            }
        }
    }

    shutdown(fp, "Byby");
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DllHandle = hModule;
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Menue, NULL, 0, NULL);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

