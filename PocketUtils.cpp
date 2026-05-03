#include "PocketUtils.h"

namespace PocketUtils {

    bool IsNpcapAvailable() {
        HMODULE hDll = LoadLibrary(L"wpcap.dll");
        if (hDll) {
            FreeLibrary(hDll);
            return true;
        }
        return false;
    }

    std::wstring GetNpcapErrorMessage() {
        return L"Npcap was not found on this system.\n\n"
               L"Verify Npcap installation and that 'WinPcap API-compatible mode' was enabled during installation.\n\n"
               L"Download available at https://npcap.com/";
    }
}
