#include "PocketUtils.h"
#include <iomanip>
#include <sstream>

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

    std::string FormatMac(const u_char* mac) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (int i = 0; i < 6; ++i) {
            ss << std::setw(2) << (int)mac[i] << (i < 5 ? ":" : "");
        }
        return ss.str();
    }

    std::string GetAppProtocol(uint16_t port) {
        if (port == 80) return "HTTP";
        if (port == 443) return "HTTPS";
        if (port == 53) return "DNS";
        if (port == 21) return "FTP";
        if (port == 22) return "SSH";
        if (port == 23) return "TELNET";
        if (port == 3389) return "RDP";
        return "";
    }

    ParsedPacket ProtocolParser::Parse(const PacketData& packet) {
        ParsedPacket parsed;
        parsed.length = packet.header.len;
        parsed.payload_offset = 0;
        parsed.payload_length = 0;
        
        char time_buf[64];
        struct tm ltm;
        time_t local_tv_sec = packet.header.ts.tv_sec;
        localtime_s(&ltm, &local_tv_sec);
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &ltm);
        sprintf_s(time_buf + strlen(time_buf), sizeof(time_buf) - strlen(time_buf), ".%06ld", packet.header.ts.tv_usec);
        parsed.timestamp = time_buf;

        if (packet.data.size() < 14) return parsed;

        const u_char* eth_header = packet.data.data();
        parsed.dest_mac = FormatMac(eth_header);
        parsed.src_mac = FormatMac(eth_header + 6);
        uint16_t eth_type = ntohs(*(uint16_t*)(eth_header + 12));

        if (eth_type == 0x0800) {
            if (packet.data.size() < 14 + 20) return parsed;
            const u_char* ip_header = eth_header + 14;
            uint8_t ihl = (*ip_header & 0x0F) * 4;
            
            char ip_src[INET_ADDRSTRLEN];
            char ip_dst[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, (void*)(ip_header + 12), ip_src, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, (void*)(ip_header + 16), ip_dst, INET_ADDRSTRLEN);
            
            parsed.src_ip = ip_src;
            parsed.dest_ip = ip_dst;
            parsed.protocol = "IPv4";

            uint8_t ip_proto = *(ip_header + 9);
            const u_char* l4_header = ip_header + ihl;
            if (packet.data.size() >= (size_t)(14 + ihl + 4)) {
                parsed.src_port = ntohs(*(uint16_t*)l4_header);
                parsed.dest_port = ntohs(*(uint16_t*)(l4_header + 2));
                
                if (ip_proto == 6) {
                    parsed.info = "TCP";
                    uint8_t tcp_len = ((*(l4_header + 12)) >> 4) * 4;
                    parsed.payload_offset = 14 + ihl + tcp_len;
                } else if (ip_proto == 17) {
                    parsed.info = "UDP";
                    parsed.payload_offset = 14 + ihl + 8;
                }

                if (parsed.payload_offset > 0 && packet.data.size() > parsed.payload_offset) {
                    parsed.payload_length = (uint32_t)packet.data.size() - parsed.payload_offset;
                }

                std::string app_proto = GetAppProtocol(parsed.src_port);
                if (app_proto.empty()) app_proto = GetAppProtocol(parsed.dest_port);
                
                if (!app_proto.empty()) parsed.info = app_proto;
                parsed.info += " " + std::to_string(parsed.src_port) + " -> " + std::to_string(parsed.dest_port);
            }
            else if (ip_proto == 1) parsed.info = "ICMP";
            else parsed.info = "Proto: " + std::to_string(ip_proto);
        }
        else if (eth_type == 0x86DD) {
            if (packet.data.size() < 14 + 40) return parsed;
            const u_char* ip_header = eth_header + 14;

            char ip_src[INET6_ADDRSTRLEN];
            char ip_dst[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, (void*)(ip_header + 8), ip_src, INET6_ADDRSTRLEN);
            inet_ntop(AF_INET6, (void*)(ip_header + 24), ip_dst, INET6_ADDRSTRLEN);

            parsed.src_ip = ip_src;
            parsed.dest_ip = ip_dst;
            parsed.protocol = "IPv6";

            uint8_t ip_proto = *(ip_header + 6);
            const u_char* l4_header = ip_header + 40;
            if (packet.data.size() >= 14 + 40 + 4) {
                parsed.src_port = ntohs(*(uint16_t*)l4_header);
                parsed.dest_port = ntohs(*(uint16_t*)(l4_header + 2));

                if (ip_proto == 6) {
                    parsed.info = "TCP";
                    uint8_t tcp_len = ((*(l4_header + 12)) >> 4) * 4;
                    parsed.payload_offset = 14 + 40 + tcp_len;
                } else if (ip_proto == 17) {
                    parsed.info = "UDP";
                    parsed.payload_offset = 14 + 40 + 8;
                }

                if (parsed.payload_offset > 0 && packet.data.size() > parsed.payload_offset) {
                    parsed.payload_length = (uint32_t)packet.data.size() - parsed.payload_offset;
                }

                std::string app_proto = GetAppProtocol(parsed.src_port);
                if (app_proto.empty()) app_proto = GetAppProtocol(parsed.dest_port);

                if (!app_proto.empty()) parsed.info = app_proto;
                parsed.info += " " + std::to_string(parsed.src_port) + " -> " + std::to_string(parsed.dest_port);
            }
            else if (ip_proto == 58) parsed.info = "ICMPv6";
            else parsed.info = "NextHeader: " + std::to_string(ip_proto);
        }
        else {
            parsed.protocol = "Ethernet";
            if (eth_type == 0x0806) parsed.info = "ARP";
            else parsed.info = "Type: 0x" + (std::stringstream() << std::hex << eth_type).str();
        }

        return parsed;
    }

    void PacketQueue::Push(const PacketData& packet) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(packet);
    }

    bool PacketQueue::Pop(PacketData& packet) {
        std::lock_guard<std::mutex> lock(mutex);
        if (queue.empty()) {
            return false;
        }
        packet = queue.front();
        queue.pop();
        return true;
    }

    void PacketQueue::Clear() {
        std::lock_guard<std::mutex> lock(mutex);
        while (!queue.empty()) {
            queue.pop();
        }
    }

    size_t PacketQueue::Size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.size();
    }

    CaptureManager::CaptureManager() : handle(nullptr), running(false) {}

    CaptureManager::~CaptureManager() {
        Stop();
    }

    bool CaptureManager::IsRunning() const {
        return running;
    }

    PacketQueue& CaptureManager::GetQueue() {
        return packet_queue;
    }

    uint32_t CaptureManager::GetDroppedCount() const {
        if (!handle) return dropped_count;
        struct pcap_stat pcs;
        if (pcap_stats(handle, &pcs) >= 0) {
            return pcs.ps_drop;
        }
        return dropped_count;
    }

    void CaptureManager::Stop() {
        if (handle) {
            struct pcap_stat pcs;
            if (pcap_stats(handle, &pcs) >= 0) {
                dropped_count = pcs.ps_drop;
            }
        }
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

        packet_queue.Clear();
        dropped_count = 0;
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

            PacketData packet;
            packet.header = *header;
            packet.data.assign(pkt_data, pkt_data + header->caplen);
            packet_queue.Push(packet);
        }
        running = false;
    }
}
