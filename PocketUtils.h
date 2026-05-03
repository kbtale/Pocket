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

    class CaptureManager {
    public:
        CaptureManager();
        ~CaptureManager();

        bool Start(const std::string& adapter_name);
        void Stop();
        bool IsRunning() const;

    private:
        void CaptureLoop();

        pcap_t* handle;
        std::thread capture_thread;
        std::atomic<bool> running;
        std::string current_adapter;
    };
}
