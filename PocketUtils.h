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

    struct PacketData {
        std::vector<u_char> data;
        struct pcap_pkthdr header;
    };

    struct ParsedPacket {
        std::string src_mac;
        std::string dest_mac;
        std::string src_ip;
        std::string dest_ip;
        std::string protocol;
        uint16_t src_port;
        uint16_t dest_port;
        uint32_t length;
        std::string timestamp;
        std::string info;
        uint32_t payload_offset;
        uint32_t payload_length;
    };

    class ProtocolParser {
    public:
        static ParsedPacket Parse(const PacketData& packet);
    };

    class PacketQueue {
    public:
        void Push(const PacketData& packet);
        bool Pop(PacketData& packet);
        void Clear();
        size_t Size() const;

    private:
        std::queue<PacketData> queue;
        mutable std::mutex mutex;
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
        PacketQueue& GetQueue();
        uint32_t GetDroppedCount() const;

    private:
        void CaptureLoop();

        pcap_t* handle;
        std::thread capture_thread;
        std::atomic<bool> running;
        std::string current_adapter;
        PacketQueue packet_queue;
        std::atomic<uint32_t> dropped_count;
    };
}
