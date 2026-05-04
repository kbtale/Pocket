#pragma once
#include "framework.h"

namespace PocketUtils {
    bool IsNpcapAvailable();
    bool IsNpcapInRegistry();
    void TriggerNpcapInstall();
    std::wstring GetNpcapErrorMessage();

    struct AdapterInfo {
        std::string name;
        std::string description;
        std::vector<std::string> addresses;
    };

    struct PacketData {
        std::vector<u_char> data;
        struct pcap_pkthdr header;
        std::string adapter;
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
        std::string adapter;
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

    class CaptureEngine {
    public:
        CaptureEngine();
        ~CaptureEngine();

        bool Start(const std::vector<std::string>& adapter_names);
        void Stop();
        bool IsRunning() const;
        void GetPackets(std::vector<PacketData>& packets);
        uint32_t GetDroppedCount() const;
        std::string GetLastError() const;

    private:
        struct HandleContext {
            pcap_t* handle;
            HANDLE event;
            std::string name;
        };

        void WorkerLoop();

        std::vector<HandleContext> contexts;
        std::thread worker_thread;
        std::atomic<bool> running;
        PacketQueue packet_queue;
        std::string last_error;
    };

    std::wstring GetPacketDetails(const PacketData& packet);
}
