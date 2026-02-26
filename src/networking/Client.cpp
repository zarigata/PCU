/**
 * @file Client.cpp
 * @brief Client implementation
 */

#include <VoxelForge/networking/NetworkManager.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

// ============================================================================
// Client Implementation
// ============================================================================

Client::Client() : NetworkManager() {
    LOG_INFO("Client created");
}

Client::~Client() {
    disconnect();
    LOG_INFO("Client destroyed");
}

bool Client::connect(const std::string& hostAddress, uint16_t port) {
    if (initialized) {
        LOG_ERROR("Client already connected");
        return false;
    }
    
    // Create ENet client
    host = enet_host_create(nullptr, 1, config.maxChannels,
                            config.incomingBandwidth, config.outgoingBandwidth);
    
    if (!host) {
        LOG_ERROR("Failed to create client");
        return false;
    }
    
    // Resolve address
    ENetAddress address;
    enet_address_set_host(&address, hostAddress.c_str());
    address.port = port;
    
    // Connect
    serverPeer = enet_host_connect(host, &address, config.maxChannels, 0);
    if (!serverPeer) {
        LOG_ERROR("Failed to initiate connection to {}:{}", hostAddress, port);
        enet_host_destroy(host);
        host = nullptr;
        return false;
    }
    
    serverHost = hostAddress;
    serverPort = port;
    initialized = true;
    state = ConnectionState::Connecting;
    
    // Wait for connection
    ENetEvent event;
    if (enet_host_service(host, &event, config.connectionTimeout) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {
        LOG_INFO("Connected to {}:{}", hostAddress, port);
        state = ConnectionState::Handshaking;
        return true;
    }
    
    LOG_ERROR("Connection to {}:{} timed out", hostAddress, port);
    enet_peer_reset(serverPeer);
    serverPeer = nullptr;
    enet_host_destroy(host);
    host = nullptr;
    initialized = false;
    state = ConnectionState::Disconnected;
    return false;
}

void Client::disconnect() {
    if (!initialized) return;
    
    if (serverPeer) {
        enet_peer_disconnect(serverPeer, 0);
        
        // Wait for disconnect acknowledgment
        ENetEvent event;
        while (enet_host_service(host, &event, 3000) > 0) {
            if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                break;
            }
        }
        
        enet_peer_reset(serverPeer);
        serverPeer = nullptr;
    }
    
    if (host) {
        enet_host_destroy(host);
        host = nullptr;
    }
    
    initialized = false;
    state = ConnectionState::Disconnected;
    
    LOG_INFO("Disconnected from server");
}

void Client::login(const std::string& username) {
    if (!isConnected()) {
        LOG_ERROR("Not connected to server");
        return;
    }
    
    PacketWriter writer(PacketType::LoginRequest);
    writer.writeString(username);
    writer.writeU32(VOXELFORGE_VERSION_MAJOR);
    writer.writeU32(VOXELFORGE_VERSION_MINOR);
    
    Packet packet;
    packet.type = PacketType::LoginRequest;
    packet.data = writer.build();
    packet.reliability = PacketReliability::Reliable;
    sendPacket(serverId, packet);
    
    LOG_INFO("Logging in as {}", username);
}

void Client::sendPosition(const glm::vec3& pos, float yaw, float pitch) {
    if (!isConnected()) return;
    
    PacketWriter writer(PacketType::PlayerPosition);
    writer.writeVec3(pos);
    writer.writeFloat(yaw);
    writer.writeFloat(pitch);
    
    Packet packet;
    packet.type = PacketType::PlayerPosition;
    packet.data = writer.build();
    packet.reliability = PacketReliability::UnreliableSequenced;
    sendPacket(serverId, packet);
}

void Client::sendBlockBreak(const glm::ivec3& pos) {
    if (!isConnected()) return;
    
    PacketWriter writer(PacketType::PlayerDigging);
    writer.writeI32(0); // Action: start break
    writer.writeIVec3(pos);
    writer.writeU8(0); // Face
    
    Packet packet;
    packet.type = PacketType::PlayerDigging;
    packet.data = writer.build();
    sendPacket(serverId, packet);
}

void Client::sendBlockPlace(const glm::ivec3& pos, uint8_t face) {
    if (!isConnected()) return;
    
    PacketWriter writer(PacketType::PlayerBlockPlacement);
    writer.writeIVec3(pos);
    writer.writeU8(face);
    writer.writeI16(0); // Held item slot
    
    Packet packet;
    packet.type = PacketType::PlayerBlockPlacement;
    packet.data = writer.build();
    sendPacket(serverId, packet);
}

void Client::sendChatMessage(const std::string& message) {
    if (!isConnected()) return;
    
    PacketWriter writer(PacketType::ChatMessage);
    writer.writeString(message);
    
    Packet packet;
    packet.type = PacketType::ChatMessage;
    packet.data = writer.build();
    sendPacket(serverId, packet);
}

void Client::sendInventoryClick(uint8_t windowId, int16_t slot, uint8_t button) {
    if (!isConnected()) return;
    
    PacketWriter writer(PacketType::SetSlot);
    writer.writeU8(windowId);
    writer.writeI16(slot);
    writer.writeU8(button);
    
    Packet packet;
    packet.type = PacketType::SetSlot;
    packet.data = writer.build();
    sendPacket(serverId, packet);
}

} // namespace VoxelForge
