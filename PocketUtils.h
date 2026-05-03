#pragma once
#include "framework.h"

namespace PocketUtils {
    bool IsNpcapAvailable();

    std::wstring GetNpcapErrorMessage();

    struct AdapterInfo {
        std::string name;
        std::string description;
        std::vector<std::string> addresses;
    };

    std::vector<AdapterInfo> GetAdapters(std::string& err);

    bool IsAdmin();

    std::wstring ConvertToWide(const std::string& str);
}
