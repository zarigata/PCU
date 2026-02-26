/**
 * @file NetworkManager.hpp
 * @brief ENet-based networking system
 */

#pragma once

#include <enet/enet.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <queue>

namespace VoxelForge {

// Forward declarations
class World;
class Entity;

// Network configuration
struct NetworkConfig {
    uint16_t port = 25565;
    uint16_t maxPlayers = 20;
    uint32_t maxChannels = 4;
    uint32_t incomingBandwidth = 0;  // 0 = unlimited
    uint32_t outgoingBandwidth = 0;
    uint32_t connectionTimeout = 5000;  // ms
    uint32_t keepAliveInterval = 1000;  // ms
    bool enableEncryption = false;
};

// Connection state
enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Disconnecting
};

// Packet types
enum class PacketType : uint8_t {
    // Handshake
    Handshake = 0,
    HandshakeResponse = 1,
    
    // Login
    LoginRequest = 10,
    LoginResponse = 11,
    Disconnect = 12,
    
    // World
    ChunkData = 20,
    ChunkUnload = 21,
    BlockChange = 22,
    MultiBlockChange = 23,
    
    // Entity
    EntitySpawn = 30,
    EntityDestroy = 31,
    EntityMove = 32,
    EntityVelocity = 33,
    EntityMetadata = 34,
    EntityAnimation = 35,
    
    // Player
    PlayerPosition = 40,
    PlayerRotation = 41,
    PlayerAbilities = 42,
    PlayerInventory = 43,
    PlayerDigging = 44,
    PlayerBlockPlacement = 45,
    PlayerHeldItem = 46,
    
    // Inventory
    WindowItems = 50,
    WindowProperty = 51,
    SetSlot = 52,
    CraftRecipe = 53,
    
    // Chat
    ChatMessage = 60,
    SystemMessage = 61,
    
    // Sound/Effects
    SoundEffect = 70,
    ParticleEffect = 71,
    Explosion = 72,
    
    // Game state
    GameStateChange = 80,
    TimeUpdate = 81,
    WeatherChange = 82,
    Respawn = 83,
    
    // Keep alive
    KeepAlive = 100,
    Ping = 101,
    Pong = 102
};

// Packet priority
enum class PacketPriority : uint8_t {
    Low = 0,
    Medium = 1,
    High = 2,
    Immediate = 3
};

// Packet reliability
enum class PacketReliability : uint8_t {
    Unreliable = 0,
    UnreliableSequenced = 1,
    Reliable = 2,
    ReliableOrdered = 3,
    ReliableSequenced = 4
};

// Network packet
struct Packet {
    PacketType type = PacketType::Handshake;
    std::vector<uint8_t> data;
    uint32_t clientId = 0;
    PacketPriority priority = PacketPriority::Medium;
    PacketReliability reliability = PacketReliability::Reliable;
    uint8_t channel = 0;
};

// Packet reader helper
class PacketReader {
public:
    PacketReader(const uint8_t* data, size_t size);
    
    uint8_t readU8();
    uint16_t readU16();
    uint32_t readU32();
    uint64_t readU64();
    int8_t readI8();
    int16_t readI16();
    int32_t readI32();
    int64_t readI64();
    float readFloat();
    double readDouble();
    bool readBool();
    std::string readString();
    glm::vec3 readVec3();
    glm::ivec3 readIVec3();
    
    size_t getRemaining() const { return size - pos; }
    const uint8_t* getData() const { return data; }
    size_t getSize() const { return size; }
    
private:
    const uint8_t* data;
    size_t size;
    size_t pos = 0;
};

// Packet writer helper
class PacketWriter {
public:
    PacketWriter(PacketType type);
    
    void writeU8(uint8_t value);
    void writeU16(uint16_t value);
    void writeU32(uint32_t value);
    void writeU64(uint64_t value);
    void writeI8(int8_t value);
    void writeI16(int16_t value);
    void writeI32(int32_t value);
    void writeI64(int64_t value);
    void writeFloat(float value);
    void writeDouble(double value);
    void writeBool(bool value);
    void writeString(const std::string& value);
    void writeVec3(const glm::vec3& value);
    void writeIVec3(const glm::ivec3& value);
    void writeBytes(const uint8_t* data, size_t size);
    
    std::vector<uint8_t> build();
    PacketType getType() const { return type; }
    
private:
    PacketType type;
    std::vector<uint8_t> buffer;
};

// Network statistics
struct NetworkStats {
    uint32_t bytesSent = 0;
    uint32_t bytesReceived = 0;
    uint32_t packetsSent = 0;
    uint32_t packetsReceived = 0;
    uint32_t packetsLost = 0;
    float pingMs = 0.0f;
    float uploadRateKBps = 0.0f;
    float downloadRateKBps = 0.0f;
};

// Client info
struct ClientInfo {
    uint32_t id = 0;
    std::string username;
    glm::vec3 position;
    float yaw = 0.0f;
    float pitch = 0.0f;
    uint8_t gamemode = 0;
    int64_t lastKeepAlive = 0;
    bool isOnline = false;
};

// Network event
struct NetworkEvent {
    enum class Type {
        Connect,
        Disconnect,
        Packet,
        Timeout
    };
    
    Type type;
    uint32_t clientId = 0;
    std::unique_ptr<Packet> packet;
    std::string message;
};

// Network callback types
using PacketCallback = std::function<void(uint32_t clientId, PacketReader& reader)>;
using ConnectionCallback = std::function<void(uint32_t clientId, bool connected)>;

// Base network manager
class NetworkManager {
public:
    NetworkManager();
    virtual ~NetworkManager();
    
    // No copy
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;
    
    void init();
    void shutdown();
    
    // Packet handling
    void registerCallback(PacketType type, PacketCallback callback);
    void unregisterCallback(PacketType type);
    
    // Send packet
    void sendPacket(uint32_t clientId, const Packet& packet);
    void broadcastPacket(const Packet& packet, uint32_t excludeClient = 0);
    void broadcastPacketToRange(const glm::vec3& center, float range, 
                                const Packet& packet, uint32_t excludeClient = 0);
    
    // Process events
    void pollEvents();
    void processPackets();
    
    // Statistics
    const NetworkStats& getStats() const { return stats; }
    void resetStats();
    
    // Getters
    bool isInitialized() const { return initialized; }
    ConnectionState getState() const { return state; }
    
protected:
    void handlePacket(uint32_t clientId, PacketType type, const uint8_t* data, size_t size);
    void onConnect(uint32_t clientId);
    void onDisconnect(uint32_t clientId);
    
    bool initialized = false;
    ConnectionState state = ConnectionState::Disconnected;
    ENetHost* host = nullptr;
    NetworkConfig config;
    NetworkStats stats;
    
    std::unordered_map<PacketType, PacketCallback> callbacks;
    std::queue<NetworkEvent> eventQueue;
    
    std::unordered_map<uint32_t, ENetPeer*> peers;
    uint32_t nextClientId = 1;
};

// Server
class Server : public NetworkManager {
public:
    Server();
    ~Server() override;
    
    bool start(uint16_t port, uint16_t maxPlayers);
    void stop();
    
    // Client management
    void kickClient(uint32_t clientId, const std::string& reason);
    void banClient(uint32_t clientId);
    void banIP(const std::string& ip);
    bool isBanned(const std::string& ip) const;
    
    const std::unordered_map<uint32_t, ClientInfo>& getClients() const { return clients; }
    uint32_t getClientCount() const { return static_cast<uint32_t>(clients.size()); }
    uint32_t getMaxPlayers() const { return config.maxPlayers; }
    
    // World sync
    void sendChunk(uint32_t clientId, int32_t x, int32_t z);
    void broadcastBlockChange(const glm::ivec3& pos, uint32_t blockState);
    void broadcastEntityMove(uint32_t entityId, const glm::vec3& pos, const glm::vec3& rot);
    void broadcastChatMessage(const std::string& sender, const std::string& message);
    void broadcastSystemMessage(const std::string& message);
    
    // Server info
    void setMotd(const std::string& motd) { this->motd = motd; }
    const std::string& getMotd() const { return motd; }
    
private:
    std::unordered_map<uint32_t, ClientInfo> clients;
    std::vector<std::string> bannedIPs;
    std::string motd = "A VoxelForge Server";
};

// Client
class Client : public NetworkManager {
public:
    Client();
    ~Client() override;
    
    bool connect(const std::string& host, uint16_t port);
    void disconnect();
    
    // Connection
    ConnectionState getConnectionState() const { return state; }
    bool isConnected() const { return state == ConnectionState::Connected; }
    
    // Login
    void login(const std::string& username);
    void setConnectionCallback(ConnectionCallback callback) { 
        connectionCallback = std::move(callback); 
    }
    
    // Player actions
    void sendPosition(const glm::vec3& pos, float yaw, float pitch);
    void sendBlockBreak(const glm::ivec3& pos);
    void sendBlockPlace(const glm::ivec3& pos, uint8_t face);
    void sendChatMessage(const std::string& message);
    void sendInventoryClick(uint8_t windowId, int16_t slot, uint8_t button);
    
    // Server info
    const std::string& getServerHost() const { return serverHost; }
    uint16_t getServerPort() const { return serverPort; }
    uint32_t getServerId() const { return serverId; }
    
private:
    std::string serverHost;
    uint16_t serverPort = 0;
    uint32_t serverId = 0;
    ENetPeer* serverPeer = nullptr;
    ConnectionCallback connectionCallback;
};

} // namespace VoxelForge
