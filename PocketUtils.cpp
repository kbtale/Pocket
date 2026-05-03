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
}
