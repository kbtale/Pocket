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

    std::vector<AdapterInfo> GetAdapters(std::string& err) {
        std::vector<AdapterInfo> adapters;
        pcap_if_t* alldevs;
        char errbuf[PCAP_ERRBUF_SIZE];

        if (pcap_findalldevs(&alldevs, errbuf) == -1) {
            err = errbuf;
            return adapters;
        }

        for (pcap_if_t* d = alldevs; d != NULL; d = d->next) {
            AdapterInfo info;
            info.name = d->name;
            info.description = d->description ? d->description : "No description available";

            for (pcap_addr_t* a = d->addresses; a != NULL; a = a->next) {
                if (a->addr->sa_family == AF_INET) {
                    char ip[INET_ADDRSTRLEN];
                    sockaddr_in* s = (sockaddr_in*)a->addr;
                    if (inet_ntop(AF_INET, &s->sin_addr, ip, sizeof(ip))) {
                        info.addresses.push_back(ip);
                    }
                }
                else if (a->addr->sa_family == AF_INET6) {
                    char ip[INET6_ADDRSTRLEN];
                    sockaddr_in6* s = (sockaddr_in6*)a->addr;
                    if (inet_ntop(AF_INET6, &s->sin6_addr, ip, sizeof(ip))) {
                        info.addresses.push_back(ip);
                    }
                }
            }
            adapters.push_back(info);
        }

        pcap_freealldevs(alldevs);
        return adapters;
    }

    bool IsAdmin() {
        BOOL result = FALSE;
        PSID admin_group = NULL;
        SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;

        if (AllocateAndInitializeSid(&nt_authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &admin_group)) {
            CheckTokenMembership(NULL, admin_group, &result);
            FreeSid(admin_group);
        }

        return result == TRUE;
    }

    std::wstring ConvertToWide(const std::string& str) {
        if (str.empty()) return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }

    CaptureManager::CaptureManager() : handle(nullptr), running(false) {}

    CaptureManager::~CaptureManager() {
        Stop();
    }

    bool CaptureManager::IsRunning() const {
        return running;
    }

    void CaptureManager::Stop() {
        running = false;
        if (capture_thread.joinable()) {
            capture_thread.join();
        }
        if (handle) {
            pcap_close(handle);
            handle = nullptr;
        }
    }

    bool CaptureManager::Start(const std::string& adapter_name) {
        if (running) {
            return false;
        }

        char errbuf[PCAP_ERRBUF_SIZE];
        handle = pcap_open_live(adapter_name.c_str(), 65535, 1, 1000, errbuf);
        if (!handle) {
            return false;
        }

        current_adapter = adapter_name;
        running = true;
        capture_thread = std::thread(&CaptureManager::CaptureLoop, this);
        return true;
    }

    void CaptureManager::CaptureLoop() {
        pcap_pkthdr* header;
        const u_char* pkt_data;

        while (running) {
            int res = pcap_next_ex(handle, &header, &pkt_data);
            if (res == 0) continue;
            if (res == -1 || res == -2) break;
        }
        running = false;
    }
}
