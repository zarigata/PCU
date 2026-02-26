/**
 * @file Connection.hpp
 * @brief Network connection management
 */

#pragma once

#include <enet/enet.h>
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <queue>

namespace VoxelForge {

class Packet;
class PacketReader;
class PacketWriter;

// Connection state
enum class ConnectionState {
    Disconnected,
    Connecting,
    Handshaking,
    LoggingIn,
    Playing,
    Disconnecting
};

// Connection statistics
struct ConnectionStats {
    uint64_t bytesSent = 0;
    uint64_t bytesReceived = 0;
    uint32_t packetsSent = 0;
    uint32_t packetsReceived = 0;
    uint32_t packetsLost = 0;
    float pingMs = 0.0f;
    float packetLoss = 0.0f;
    float uploadRate = 0.0f;       // bytes/sec
    float downloadRate = 0.0f;     // bytes/sec
    std::chrono::steady_clock::time_point lastPingTime;
    std::chrono::steady_clock::time_point lastPacketTime;
};

// Connection configuration
struct ConnectionConfig {
    uint32_t timeout = 5000;           // Connection timeout in ms
    uint32_t keepAliveInterval = 1000; // Keep-alive interval in ms
    uint32_t retryCount = 3;           // Connection retry count
    uint32_t retryDelay = 1000;        // Delay between retries in ms
    uint32_t maxPacketSize = 65536;    // Maximum packet size
    bool enableCompression = true;     // Enable packet compression
    uint32_t compressionThreshold = 256; // Compress packets larger than this
    bool enableEncryption = false;     // Enable encryption (future)
};

// Connection event
struct ConnectionEvent {
    enum class Type {
        Connected,
        Disconnected,
        Timeout,
        PacketReceived,
        PacketSent,
        Error
    };
    
    Type type;
    std::string message;
    std::unique_ptr<Packet> packet;
};

// Network connection
class Connection {
public:
    Connection();
    ~Connection();
    
    // No copy
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    
    // Connection management
    bool connect(ENetHost* host, const std::string& address, uint16_t port);
    void disconnect();
    void disconnect(const std::string& reason);
    void forceDisconnect();
    
    // State
    ConnectionState getState() const { return state; }
    bool isConnected() const { return state == ConnectionState::Playing; }
    bool isConnecting() const { return state == ConnectionState::Connecting || 
                                       state == ConnectionState::Handshaking ||
                                       state == ConnectionState::LoggingIn; }
    
    // Send
    void sendPacket(const Packet& packet);
    void sendPacketImmediate(const Packet& packet);
    void sendPacketReliable(const Packet& packet);
    void sendPacketUnreliable(const Packet& packet);
    
    // Receive
    std::unique_ptr<Packet> receivePacket();
    bool hasPendingPackets() const { return !incomingPackets.empty(); }
    
    // Update
    void update();
    void processEvents();
    
    // Statistics
    const ConnectionStats& getStats() const { return stats; }
    void resetStats();
    float getPing() const { return stats.pingMs; }
    
    // Keep-alive
    void sendKeepAlive();
    void checkTimeout();
    
    // Configuration
    void setConfig(const ConnectionConfig& config) { this->config = config; }
    const ConnectionConfig& getConfig() const { return config; }
    
    // Callbacks
    using EventCallback = std::function<void(const ConnectionEvent&)>;
    void setEventCallback(EventCallback callback) { eventCallback = std::move(callback); }
    
    // Peer info
    uint32_t getId() const { return peerId; }
    const std::string& getAddress() const { return address; }
    uint16_t getPort() const { return port; }
    ENetPeer* getPeer() const { return peer; }
    
    // Compression
    void enableCompression(bool enable) { config.enableCompression = enable; }
    void setCompressionThreshold(uint32_t threshold) { config.compressionThreshold = threshold; }
    
private:
    void handleConnect();
    void handleDisconnect();
    void handlePacket(ENetPacket* packet);
    void updateStats();
    void updateRates();
    
    ENetPeer* peer = nullptr;
    uint32_t peerId = 0;
    std::string address;
    uint16_t port = 0;
    
    ConnectionState state = ConnectionState::Disconnected;
    ConnectionConfig config;
    ConnectionStats stats;
    
    std::queue<std::unique_ptr<Packet>> incomingPackets;
    std::queue<std::unique_ptr<Packet>> outgoingPackets;
    
    EventCallback eventCallback;
    
    // Rate calculation
    uint64_t lastBytesSent = 0;
    uint64_t lastBytesReceived = 0;
    std::chrono::steady_clock::time_point lastRateUpdate;
    
    // Keep-alive
    std::chrono::steady_clock::time_point lastKeepAliveSent;
    std::chrono::steady_clock::time_point lastKeepAliveReceived;
};

// Connection factory
class ConnectionFactory {
public:
    static std::unique_ptr<Connection> create();
    static std::unique_ptr<Connection> createAndConnect(ENetHost* host, 
                                                         const std::string& address, 
                                                         uint16_t port);
};

} // namespace VoxelForge
