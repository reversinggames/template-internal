#include <windows.h>
#include <cstdio>


void eject_thread(HMODULE module_base) {
    Sleep(100);
    FreeLibraryAndExitThread(module_base, 0);
}


void shutdown(HMODULE module_base, FILE* fp) {
    if (fp != nullptr) { fclose(fp); }
    FreeConsole();
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)eject_thread, module_base, 0, 0);
}


void main_thread(HMODULE module_base) {
    // Enable console
    FILE* fp;
    AllocConsole();
    freopen_s(&fp, "CONOUT$", "w", stdout);

    // Wait for delete to leave
    printf("[-] Injected!\n");
    while (true) {
        if (GetAsyncKeyState(VK_DELETE)) {
            break;
        }
        Sleep(10);
    }

    // Leave
    printf("[-] Leaving...\n");
    shutdown(module_base, fp);
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  reason_for_call, LPVOID lpReserved) {
    if (reason_for_call == DLL_PROCESS_ATTACH) {
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main_thread, hModule, NULL, NULL);
    }
    return TRUE;
}
