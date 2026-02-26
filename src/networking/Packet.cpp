/**
 * @file Packet.cpp
 * @brief Packet reader/writer implementation
 */

#include <VoxelForge/networking/NetworkManager.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <cstring>

namespace VoxelForge {

// ============================================================================
// PacketReader Implementation
// ============================================================================

PacketReader::PacketReader(const uint8_t* data, size_t size)
    : data(data), size(size), pos(0) {}

uint8_t PacketReader::readU8() {
    if (pos + 1 > size) {
        LOG_ERROR("Packet read overflow at pos {}", pos);
        return 0;
    }
    return data[pos++];
}

uint16_t PacketReader::readU16() {
    if (pos + 2 > size) {
        LOG_ERROR("Packet read overflow at pos {}", pos);
        return 0;
    }
    uint16_t value = static_cast<uint16_t>(data[pos]) |
                     (static_cast<uint16_t>(data[pos + 1]) << 8);
    pos += 2;
    return value;
}

uint32_t PacketReader::readU32() {
    if (pos + 4 > size) {
        LOG_ERROR("Packet read overflow at pos {}", pos);
        return 0;
    }
    uint32_t value = static_cast<uint32_t>(data[pos]) |
                     (static_cast<uint32_t>(data[pos + 1]) << 8) |
                     (static_cast<uint32_t>(data[pos + 2]) << 16) |
                     (static_cast<uint32_t>(data[pos + 3]) << 24);
    pos += 4;
    return value;
}

uint64_t PacketReader::readU64() {
    if (pos + 8 > size) {
        LOG_ERROR("Packet read overflow at pos {}", pos);
        return 0;
    }
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value |= static_cast<uint64_t>(data[pos + i]) << (i * 8);
    }
    pos += 8;
    return value;
}

int8_t PacketReader::readI8() {
    return static_cast<int8_t>(readU8());
}

int16_t PacketReader::readI16() {
    return static_cast<int16_t>(readU16());
}

int32_t PacketReader::readI32() {
    return static_cast<int32_t>(readU32());
}

int64_t PacketReader::readI64() {
    return static_cast<int64_t>(readU64());
}

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

bool PacketReader::readBool() {
    return readU8() != 0;
}

std::string PacketReader::readString() {
    uint16_t length = readU16();
    if (pos + length > size) {
        LOG_ERROR("Packet string read overflow at pos {}", pos);
        return "";
    }
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

// ============================================================================
// PacketWriter Implementation
// ============================================================================

PacketWriter::PacketWriter(PacketType type) : type(type) {
    buffer.reserve(256);
    // Reserve space for type
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
    for (int i = 0; i < 8; ++i) {
        buffer.push_back((value >> (i * 8)) & 0xFF);
    }
}

void PacketWriter::writeI8(int8_t value) {
    writeU8(static_cast<uint8_t>(value));
}

void PacketWriter::writeI16(int16_t value) {
    writeU16(static_cast<uint16_t>(value));
}

void PacketWriter::writeI32(int32_t value) {
    writeU32(static_cast<uint32_t>(value));
}

void PacketWriter::writeI64(int64_t value) {
    writeU64(static_cast<uint64_t>(value));
}

void PacketWriter::writeFloat(float value) {
    uint32_t intVal;
    std::memcpy(&intVal, &value, sizeof(float));
    writeU32(intVal);
}

void PacketWriter::writeDouble(double value) {
    uint64_t intVal;
    std::memcpy(&intVal, &value, sizeof(double));
    writeU64(intVal);
}

void PacketWriter::writeBool(bool value) {
    writeU8(value ? 1 : 0);
}

void PacketWriter::writeString(const std::string& value) {
    uint16_t length = static_cast<uint16_t>(std::min(value.size(), size_t(UINT16_MAX)));
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

// ============================================================================
// NetworkManager Implementation
// ============================================================================

NetworkManager::NetworkManager() {
    enet_initialize();
}

NetworkManager::~NetworkManager() {
    shutdown();
    enet_deinitialize();
}

void NetworkManager::init() {
    if (initialized) return;
    initialized = true;
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
    
    initialized = false;
    state = ConnectionState::Disconnected;
}

void NetworkManager::registerCallback(PacketType type, PacketCallback callback) {
    callbacks[type] = std::move(callback);
}

void NetworkManager::unregisterCallback(PacketType type) {
    callbacks.erase(type);
}

void NetworkManager::sendPacket(uint32_t clientId, const Packet& packet) {
    auto it = peers.find(clientId);
    if (it == peers.end() || !it->second) {
        LOG_ERROR("Invalid client ID: {}", clientId);
        return;
    }
    
    uint32_t flags = 0;
    switch (packet.reliability) {
        case PacketReliability::Reliable:
        case PacketReliability::ReliableOrdered:
            flags = ENET_PACKET_FLAG_RELIABLE;
            break;
        case PacketReliability::UnreliableSequenced:
        case PacketReliability::ReliableSequenced:
            flags = ENET_PACKET_FLAG_UNSEQUENCED;
            break;
        default:
            break;
    }
    
    ENetPacket* enetPacket = enet_packet_create(packet.data.data(),
                                                 packet.data.size(),
                                                 flags);
    if (enetPacket) {
        enet_peer_send(it->second, packet.channel, enetPacket);
        stats.packetsSent++;
        stats.bytesSent += packet.data.size();
    }
}

void NetworkManager::broadcastPacket(const Packet& packet, uint32_t excludeClient) {
    for (const auto& [id, peer] : peers) {
        if (id != excludeClient && peer) {
            Packet p = packet;
            p.clientId = id;
            sendPacket(id, p);
        }
    }
}

void NetworkManager::broadcastPacketToRange(const glm::vec3& center, float range,
                                            const Packet& packet, uint32_t excludeClient) {
    // TODO: Get clients in range and broadcast
    broadcastPacket(packet, excludeClient);
}

void NetworkManager::pollEvents() {
    if (!host) return;
    
    ENetEvent event;
    while (enet_host_service(host, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                uint32_t clientId = nextClientId++;
                event.peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(clientId));
                peers[clientId] = event.peer;
                onConnect(clientId);
                break;
            }
            
            case ENET_EVENT_TYPE_DISCONNECT: {
                uint32_t clientId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event.peer->data));
                onDisconnect(clientId);
                peers.erase(clientId);
                break;
            }
            
            case ENET_EVENT_TYPE_RECEIVE: {
                uint32_t clientId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event.peer->data));
                if (event.packet && event.packet->dataLength > 0) {
                    PacketType type = static_cast<PacketType>(event.packet->data[0]);
                    handlePacket(clientId, type, event.packet->data + 1,
                                event.packet->dataLength - 1);
                    stats.packetsReceived++;
                    stats.bytesReceived += event.packet->dataLength;
                }
                enet_packet_destroy(event.packet);
                break;
            }
            
            default:
                break;
        }
    }
}

void NetworkManager::processPackets() {
    while (!eventQueue.empty()) {
        NetworkEvent event = std::move(eventQueue.front());
        eventQueue.pop();
        
        if (event.type == NetworkEvent::Type::Packet && event.packet) {
            auto it = callbacks.find(event.packet->type);
            if (it != callbacks.end()) {
                PacketReader reader(event.packet->data.data(), event.packet->data.size());
                it->second(event.clientId, reader);
            }
        }
    }
}

void NetworkManager::resetStats() {
    stats = NetworkStats{};
}

void NetworkManager::handlePacket(uint32_t clientId, PacketType type,
                                   const uint8_t* data, size_t size) {
    NetworkEvent event;
    event.type = NetworkEvent::Type::Packet;
    event.clientId = clientId;
    event.packet = std::make_unique<Packet>();
    event.packet->type = type;
    event.packet->data.assign(data, data + size);
    event.packet->clientId = clientId;
    eventQueue.push(std::move(event));
}

void NetworkManager::onConnect(uint32_t clientId) {
    LOG_INFO("Client {} connected", clientId);
    
    NetworkEvent event;
    event.type = NetworkEvent::Type::Connect;
    event.clientId = clientId;
    eventQueue.push(std::move(event));
}

void NetworkManager::onDisconnect(uint32_t clientId) {
    LOG_INFO("Client {} disconnected", clientId);
    
    NetworkEvent event;
    event.type = NetworkEvent::Type::Disconnect;
    event.clientId = clientId;
    eventQueue.push(std::move(event));
}

} // namespace VoxelForge
