/**
 * @file NetworkManager.cpp
 * @brief ENet-based networking implementation
 */

#include "NetworkManager.hpp"
#include <VoxelForge/core/Logger.hpp>
#include <cstring>
#include <algorithm>

namespace VoxelForge {

// ============== PacketReader ==============

PacketReader::PacketReader(const uint8_t* data, size_t size)
    : data(data), size(size), pos(0) {}

uint8_t PacketReader::readU8() {
    if (pos + 1 > size) return 0;
    return data[pos++];
}

uint16_t PacketReader::readU16() {
    if (pos + 2 > size) return 0;
    uint16_t value = static_cast<uint16_t>(data[pos]) |
                    (static_cast<uint16_t>(data[pos + 1]) << 8);
    pos += 2;
    return value;
}

uint32_t PacketReader::readU32() {
    if (pos + 4 > size) return 0;
    uint32_t value = static_cast<uint32_t>(data[pos]) |
                    (static_cast<uint32_t>(data[pos + 1]) << 8) |
                    (static_cast<uint32_t>(data[pos + 2]) << 16) |
                    (static_cast<uint32_t>(data[pos + 3]) << 24);
    pos += 4;
    return value;
}

uint64_t PacketReader::readU64() {
    if (pos + 8 > size) return 0;
    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= static_cast<uint64_t>(data[pos + i]) << (i * 8);
    }
    pos += 8;
    return value;
}

int8_t PacketReader::readI8() { return static_cast<int8_t>(readU8()); }
int16_t PacketReader::readI16() { return static_cast<int16_t>(readU16()); }
int32_t PacketReader::readI32() { return static_cast<int32_t>(readU32()); }
int64_t PacketReader::readI64() { return static_cast<int64_t>(readU64()); }

float PacketReader::readFloat() {
    uint32_t value = readU32();
    float result;
    std::memcpy(&result, &value, sizeof(float));
    return result;
}

double PacketReader::readDouble() {
    uint64_t value = readU64();
    double result;
    std::memcpy(&result, &value, sizeof(double));
    return result;
}

bool PacketReader::readBool() { return readU8() != 0; }

std::string PacketReader::readString() {
    uint16_t length = readU16();
    if (pos + length > size) return "";
    std::string result(reinterpret_cast<const char*>(data + pos), length);
    pos += length;
    return result;
}

glm::vec3 PacketReader::readVec3() {
    return glm::vec3(readFloat(), readFloat(), readFloat());
}

glm::ivec3 PacketReader::readIVec3() {
    return glm::ivec3(readI32(), readI32(), readI32());
}

// ============== PacketWriter ==============

PacketWriter::PacketWriter(PacketType type) : type(type) {
    buffer.reserve(64);
    writeU8(static_cast<uint8_t>(type));
}

void PacketWriter::writeU8(uint8_t value) {
    buffer.push_back(value);
}

void PacketWriter::writeU16(uint16_t value) {
    buffer.push_back(value & 0xFF);
    buffer.push_back((value >> 8) & 0xFF);
}

void PacketWriter::writeU32(uint32_t value) {
    buffer.push_back(value & 0xFF);
    buffer.push_back((value >> 8) & 0xFF);
    buffer.push_back((value >> 16) & 0xFF);
    buffer.push_back((value >> 24) & 0xFF);
}

void PacketWriter::writeU64(uint64_t value) {
    for (int i = 0; i < 8; i++) {
        buffer.push_back((value >> (i * 8)) & 0xFF);
    }
}

void PacketWriter::writeI8(int8_t value) { writeU8(static_cast<uint8_t>(value)); }
void PacketWriter::writeI16(int16_t value) { writeU16(static_cast<uint16_t>(value)); }
void PacketWriter::writeI32(int32_t value) { writeU32(static_cast<uint32_t>(value)); }
void PacketWriter::writeI64(int64_t value) { writeU64(static_cast<uint64_t>(value)); }

void PacketWriter::writeFloat(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(float));
    writeU32(bits);
}

void PacketWriter::writeDouble(double value) {
    uint64_t bits;
    std::memcpy(&bits, &value, sizeof(double));
    writeU64(bits);
}

void PacketWriter::writeBool(bool value) { writeU8(value ? 1 : 0); }

void PacketWriter::writeString(const std::string& value) {
    uint16_t length = static_cast<uint16_t>(std::min(value.length(), size_t(65535)));
    writeU16(length);
    buffer.insert(buffer.end(), value.begin(), value.begin() + length);
}

void PacketWriter::writeVec3(const glm::vec3& value) {
    writeFloat(value.x);
    writeFloat(value.y);
    writeFloat(value.z);
}

void PacketWriter::writeIVec3(const glm::ivec3& value) {
    writeI32(value.x);
    writeI32(value.y);
    writeI32(value.z);
}

void PacketWriter::writeBytes(const uint8_t* data, size_t size) {
    buffer.insert(buffer.end(), data, data + size);
}

std::vector<uint8_t> PacketWriter::build() {
    return std::move(buffer);
}

// ============== NetworkManager ==============

NetworkManager::NetworkManager() = default;

NetworkManager::~NetworkManager() {
    shutdown();
}

void NetworkManager::init() {
    if (initialized) return;
    
    if (enet_initialize() != 0) {
        Logger::error("Failed to initialize ENet");
        return;
    }
    
    initialized = true;
    Logger::info("NetworkManager initialized");
}

void NetworkManager::shutdown() {
    if (!initialized) return;
    
    if (host) {
        enet_host_destroy(host);
        host = nullptr;
    }
    
    peers.clear();
    callbacks.clear();
    
    while (!eventQueue.empty()) {
        eventQueue.pop();
    }
    
    enet_deinitialize();
    initialized = false;
    
    Logger::info("NetworkManager shutdown");
}

void NetworkManager::registerCallback(PacketType type, PacketCallback callback) {
    callbacks[type] = std::move(callback);
}

void NetworkManager::unregisterCallback(PacketType type) {
    callbacks.erase(type);
}

void NetworkManager::sendPacket(uint32_t clientId, const Packet& packet) {
    auto it = peers.find(clientId);
    if (it == peers.end() || !it->second) return;
    
    ENetPacket* enetPacket = enet_packet_create(
        packet.data.data(),
        packet.data.size(),
        static_cast<enet_uint32>(packet.reliability) |
        (packet.priority == PacketPriority::Immediate ? ENET_PACKET_FLAG_IMMEDIATE : 0)
    );
    
    if (enetPacket) {
        enet_peer_send(it->second, packet.channel, enetPacket);
        stats.packetsSent++;
        stats.bytesSent += static_cast<uint32_t>(packet.data.size());
    }
}

void NetworkManager::broadcastPacket(const Packet& packet, uint32_t excludeClient) {
    ENetPacket* enetPacket = enet_packet_create(
        packet.data.data(),
        packet.data.size(),
        static_cast<enet_uint32>(packet.reliability)
    );
    
    if (enetPacket) {
        for (const auto& [id, peer] : peers) {
            if (id != excludeClient && peer) {
                enet_peer_send(peer, packet.channel, enetPacket);
            }
        }
        stats.packetsSent++;
        stats.bytesSent += static_cast<uint32_t>(packet.data.size() * peers.size());
    }
}

void NetworkManager::broadcastPacketToRange(const glm::vec3& center, float range,
                                             const Packet& packet, uint32_t excludeClient) {
    // Would need player positions to implement properly
    broadcastPacket(packet, excludeClient);
}

void NetworkManager::pollEvents() {
    if (!host) return;
    
    ENetEvent event;
    while (enet_host_service(host, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                onConnect(event.peer);
                break;
                
            case ENET_EVENT_TYPE_DISCONNECT:
                onDisconnect(reinterpret_cast<uintptr_t>(event.peer->data));
                break;
                
            case ENET_EVENT_TYPE_RECEIVE: {
                uint32_t clientId = reinterpret_cast<uintptr_t>(event.peer->data);
                
                if (event.packet->dataLength > 0) {
                    PacketType type = static_cast<PacketType>(event.packet->data[0]);
                    handlePacket(clientId, type, 
                                event.packet->data + 1,
                                event.packet->dataLength - 1);
                }
                
                stats.packetsReceived++;
                stats.bytesReceived += static_cast<uint32_t>(event.packet->dataLength);
                enet_packet_destroy(event.packet);
                break;
            }
            
            default:
                break;
        }
    }
}

void NetworkManager::processPackets() {
    pollEvents();
}

void NetworkManager::handlePacket(uint32_t clientId, PacketType type,
                                   const uint8_t* data, size_t size) {
    auto it = callbacks.find(type);
    if (it != callbacks.end()) {
        PacketReader reader(data, size);
        it->second(clientId, reader);
    }
}

void NetworkManager::onConnect(uint32_t clientId) {
    state = ConnectionState::Connected;
    Logger::info("Client {} connected", clientId);
}

void NetworkManager::onDisconnect(uint32_t clientId) {
    peers.erase(clientId);
    Logger::info("Client {} disconnected", clientId);
}

void NetworkManager::resetStats() {
    stats = {};
}

// ============== Server ==============

Server::Server() : NetworkManager() {}

Server::~Server() {
    stop();
}

bool Server::start(uint16_t port, uint16_t maxPlayers) {
    if (!initialized) init();
    
    config.port = port;
    config.maxPlayers = maxPlayers;
    
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;
    
    host = enet_host_create(&address, maxPlayers, config.maxChannels,
                           config.incomingBandwidth, config.outgoingBandwidth);
    
    if (!host) {
        Logger::error("Failed to create server on port {}", port);
        return false;
    }
    
    state = ConnectionState::Connected;
    Logger::info("Server started on port {} with max {} players", port, maxPlayers);
    return true;
}

void Server::stop() {
    // Disconnect all clients
    for (auto& [id, peer] : peers) {
        if (peer) {
            enet_peer_disconnect(peer, 0);
        }
    }
    
    // Process remaining events
    pollEvents();
    
    if (host) {
        enet_host_destroy(host);
        host = nullptr;
    }
    
    clients.clear();
    state = ConnectionState::Disconnected;
    Logger::info("Server stopped");
}

void Server::kickClient(uint32_t clientId, const std::string& reason) {
    auto it = peers.find(clientId);
    if (it != peers.end() && it->second) {
        // Send disconnect reason
        PacketWriter writer(PacketType::Disconnect);
        writer.writeString(reason);
        
        Packet packet;
        packet.data = writer.build();
        sendPacket(clientId, packet);
        
        enet_peer_disconnect(it->second, 0);
    }
}

void Server::banClient(uint32_t clientId) {
    auto clientIt = clients.find(clientId);
    if (clientIt != clients.end()) {
        // Would need IP to ban
        kickClient(clientId, "You have been banned");
    }
}

void Server::banIP(const std::string& ip) {
    bannedIPs.push_back(ip);
}

bool Server::isBanned(const std::string& ip) const {
    return std::find(bannedIPs.begin(), bannedIPs.end(), ip) != bannedIPs.end();
}

void Server::sendChunk(uint32_t clientId, int32_t x, int32_t z) {
    PacketWriter writer(PacketType::ChunkData);
    writer.writeI32(x);
    writer.writeI32(z);
    // Would add chunk data here
    
    Packet packet;
    packet.data = writer.build();
    sendPacket(clientId, packet);
}

void Server::broadcastBlockChange(const glm::ivec3& pos, uint32_t blockState) {
    PacketWriter writer(PacketType::BlockChange);
    writer.writeIVec3(pos);
    writer.writeU32(blockState);
    
    Packet packet;
    packet.data = writer.build();
    broadcastPacket(packet);
}

void Server::broadcastEntityMove(uint32_t entityId, const glm::vec3& pos, const glm::vec3& rot) {
    PacketWriter writer(PacketType::EntityMove);
    writer.writeU32(entityId);
    writer.writeVec3(pos);
    writer.writeVec3(rot);
    
    Packet packet;
    packet.data = writer.build();
    broadcastPacket(packet);
}

void Server::broadcastChatMessage(const std::string& sender, const std::string& message) {
    PacketWriter writer(PacketType::ChatMessage);
    writer.writeString(sender);
    writer.writeString(message);
    
    Packet packet;
    packet.data = writer.build();
    broadcastPacket(packet);
}

void Server::broadcastSystemMessage(const std::string& message) {
    PacketWriter writer(PacketType::SystemMessage);
    writer.writeString(message);
    
    Packet packet;
    packet.data = writer.build();
    broadcastPacket(packet);
}

// ============== Client ==============

Client::Client() : NetworkManager() {}

Client::~Client() {
    disconnect();
}

bool Client::connect(const std::string& host, uint16_t port) {
    if (!initialized) init();
    
    this->serverHost = host;
    this->serverPort = port;
    
    host = enet_host_create(nullptr, 1, config.maxChannels,
                           config.incomingBandwidth, config.outgoingBandwidth);
    
    if (!host) {
        Logger::error("Failed to create client");
        return false;
    }
    
    ENetAddress address;
    enet_address_set_host(&address, host.c_str());
    address.port = port;
    
    serverPeer = enet_host_connect(host, &address, config.maxChannels, 0);
    
    if (!serverPeer) {
        Logger::error("Failed to initiate connection to {}:{}", host, port);
        return false;
    }
    
    state = ConnectionState::Connecting;
    
    // Wait for connection
    ENetEvent event;
    if (enet_host_service(host, &event, config.connectionTimeout) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {
        
        serverPeer->data = reinterpret_cast<void*>(1);
        serverId = 1;
        peers[serverId] = serverPeer;
        state = ConnectionState::Connected;
        
        Logger::info("Connected to {}:{}", host, port);
        return true;
    }
    
    Logger::error("Connection to {}:{} timed out", host, port);
    enet_peer_reset(serverPeer);
    state = ConnectionState::Disconnected;
    return false;
}

void Client::disconnect() {
    if (serverPeer && state == ConnectionState::Connected) {
        enet_peer_disconnect(serverPeer, 0);
        
        ENetEvent event;
        while (enet_host_service(host, &event, 3000) > 0) {
            if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                break;
            }
        }
    }
    
    if (host) {
        enet_host_destroy(host);
        host = nullptr;
    }
    
    serverPeer = nullptr;
    peers.clear();
    state = ConnectionState::Disconnected;
    Logger::info("Disconnected from server");
}

void Client::login(const std::string& username) {
    PacketWriter writer(PacketType::LoginRequest);
    writer.writeString(username);
    writer.writeU32(0);  // Protocol version
    
    Packet packet;
    packet.data = writer.build();
    sendPacket(serverId, packet);
}

void Client::sendPosition(const glm::vec3& pos, float yaw, float pitch) {
    PacketWriter writer(PacketType::PlayerPosition);
    writer.writeVec3(pos);
    writer.writeFloat(yaw);
    writer.writeFloat(pitch);
    
    Packet packet;
    packet.data = writer.build();
    packet.priority = PacketPriority::High;
    packet.reliability = PacketReliability::UnreliableSequenced;
    sendPacket(serverId, packet);
}

void Client::sendBlockBreak(const glm::ivec3& pos) {
    PacketWriter writer(PacketType::PlayerDigging);
    writer.writeU8(0);  // Start digging
    writer.writeIVec3(pos);
    writer.writeU8(0);  // Face
    
    Packet packet;
    packet.data = writer.build();
    sendPacket(serverId, packet);
}

void Client::sendBlockPlace(const glm::ivec3& pos, uint8_t face) {
    PacketWriter writer(PacketType::PlayerBlockPlacement);
    writer.writeIVec3(pos);
    writer.writeU8(face);
    
    Packet packet;
    packet.data = writer.build();
    sendPacket(serverId, packet);
}

void Client::sendChatMessage(const std::string& message) {
    PacketWriter writer(PacketType::ChatMessage);
    writer.writeString(message);
    
    Packet packet;
    packet.data = writer.build();
    sendPacket(serverId, packet);
}

void Client::sendInventoryClick(uint8_t windowId, int16_t slot, uint8_t button) {
    PacketWriter writer(PacketType::SetSlot);
    writer.writeU8(windowId);
    writer.writeI16(slot);
    writer.writeU8(button);
    
    Packet packet;
    packet.data = writer.build();
    sendPacket(serverId, packet);
}

} // namespace VoxelForge
