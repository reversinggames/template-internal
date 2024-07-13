#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>


HANDLE get_process_by_name(const wchar_t* name) {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "[ERROR]: Failed to create snapshot" << std::endl;
        return NULL;
    }
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hProcessSnap, &pe32)) {
        std::cerr << "[ERROR]: Cannot retrieve processes" << std::endl;
        CloseHandle(hProcessSnap);
        return NULL;
    }
    do {
        if (wcscmp(pe32.szExeFile, name) == 0) {
            CloseHandle(hProcessSnap);
            return OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
        }
    } while (Process32Next(hProcessSnap, &pe32));
    CloseHandle(hProcessSnap);
    return NULL;
}


bool inject_dll(HANDLE process, const wchar_t* dllpath) {
    // Allocate memory in process for dll path
    void* dllpath_mem = VirtualAllocEx(process, nullptr, 1024, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (dllpath_mem == nullptr) {
        std::cerr << "[ERROR]: Failed to allocate memory in process for path" << std::endl;
        return false;
    }

    // Write path to dll in process memory
    bool res = WriteProcessMemory(process, dllpath_mem, dllpath, (wcslen(dllpath) + 1) * sizeof(wchar_t), nullptr);
    if (!res) {
        std::cerr << "[ERROR]: Failed to write path to process memory" << std::endl;
        VirtualFreeEx(process, dllpath_mem, 0, MEM_RELEASE);
        return false;
    }

    // Create remote thread to inject dll
    HANDLE thread = CreateRemoteThread(process, nullptr, 0, (LPTHREAD_START_ROUTINE)LoadLibraryW, dllpath_mem, 0, nullptr);
    if (!thread) {
        VirtualFreeEx(process, dllpath_mem, 0, MEM_RELEASE);
        return false;
    }

    std::cout << "[INFO]: Injected dll" << std::endl;

    // Wait for startup thread to finish
    DWORD exit_code = 0;
    if (WaitForSingleObject(thread, 6000) == WAIT_OBJECT_0) {
        GetExitCodeThread(thread, (DWORD*)&exit_code);
    }
    CloseHandle(thread);
    VirtualFreeEx(process, dllpath_mem, 0, MEM_RELEASE);

    std::cout << "[INFO]: Exit code: " << exit_code << std::endl;
    return exit_code != 0;
}



int main() {
    // Get dll full path
    wchar_t dll_path[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, dll_path, MAX_PATH);
    wcscpy_s(wcsrchr(dll_path, L'\\') + 1, 64, L"mod.dll");

    // Check if actually exists
    if (GetFileAttributes(dll_path) == INVALID_FILE_ATTRIBUTES) {
        printf("dll not found, leaving...");
        return 1;
    }
    printf("Found dll %ws\n", dll_path);

    HANDLE process = get_process_by_name(L"kenshi_x64.exe");
    if (process == NULL) {
        printf("Failed to find process for kenshi, leaving...\n");
        return 2;
    }

    inject_dll(process, dll_path);

    return 0;
}