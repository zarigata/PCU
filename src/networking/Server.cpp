/**
 * @file Server.cpp
 * @brief Server implementation
 */

#include <VoxelForge/networking/NetworkManager.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

// ============================================================================
// Server Implementation
// ============================================================================

Server::Server() : NetworkManager() {
    LOG_INFO("Server created");
}

Server::~Server() {
    stop();
    LOG_INFO("Server destroyed");
}

bool Server::start(uint16_t port, uint16_t maxPlayers) {
    if (initialized) {
        LOG_ERROR("Server already running");
        return false;
    }
    
    config.port = port;
    config.maxPlayers = maxPlayers;
    
    // Create ENet server
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;
    
    host = enet_host_create(&address, maxPlayers, config.maxChannels,
                            config.incomingBandwidth, config.outgoingBandwidth);
    
    if (!host) {
        LOG_ERROR("Failed to create server on port {}", port);
        return false;
    }
    
    initialized = true;
    state = ConnectionState::Connected;
    
    LOG_INFO("Server started on port {} with max {} players", port, maxPlayers);
    return true;
}

void Server::stop() {
    if (!initialized) return;
    
    // Disconnect all clients
    for (auto& [id, client] : clients) {
        if (client.isOnline) {
            kickClient(id, "Server shutting down");
        }
    }
    
    clients.clear();
    bannedIPs.clear();
    
    if (host) {
        enet_host_destroy(host);
        host = nullptr;
    }
    
    initialized = false;
    state = ConnectionState::Disconnected;
    
    LOG_INFO("Server stopped");
}

void Server::kickClient(uint32_t clientId, const std::string& reason) {
    auto it = clients.find(clientId);
    if (it == clients.end()) return;
    
    // Send disconnect packet
    PacketWriter writer(PacketType::Disconnect);
    writer.writeString(reason);
    
    Packet packet;
    packet.type = PacketType::Disconnect;
    packet.data = writer.build();
    sendPacket(clientId, packet);
    
    // Disconnect peer
    auto peerIt = peers.find(clientId);
    if (peerIt != peers.end() && peerIt->second) {
        enet_peer_disconnect(peerIt->second, 0);
    }
    
    LOG_INFO("Kicked client {}: {}", clientId, reason);
}

void Server::banClient(uint32_t clientId) {
    auto it = clients.find(clientId);
    if (it == clients.end()) return;
    
    // TODO: Get IP from client and ban
    kickClient(clientId, "You have been banned");
}

void Server::banIP(const std::string& ip) {
    if (!isBanned(ip)) {
        bannedIPs.push_back(ip);
        LOG_INFO("Banned IP: {}", ip);
    }
}

bool Server::isBanned(const std::string& ip) const {
    return std::find(bannedIPs.begin(), bannedIPs.end(), ip) != bannedIPs.end();
}

void Server::sendChunk(uint32_t clientId, int32_t x, int32_t z) {
    PacketWriter writer(PacketType::ChunkData);
    writer.writeI32(x);
    writer.writeI32(z);
    // TODO: Write chunk data
    
    Packet packet;
    packet.type = PacketType::ChunkData;
    packet.data = writer.build();
    sendPacket(clientId, packet);
}

void Server::broadcastBlockChange(const glm::ivec3& pos, uint32_t blockState) {
    PacketWriter writer(PacketType::BlockChange);
    writer.writeIVec3(pos);
    writer.writeU32(blockState);
    
    Packet packet;
    packet.type = PacketType::BlockChange;
    packet.data = writer.build();
    broadcastPacket(packet);
}

void Server::broadcastEntityMove(uint32_t entityId, const glm::vec3& pos, const glm::vec3& rot) {
    PacketWriter writer(PacketType::EntityMove);
    writer.writeU32(entityId);
    writer.writeVec3(pos);
    writer.writeVec3(rot);
    
    Packet packet;
    packet.type = PacketType::EntityMove;
    packet.data = writer.build();
    broadcastPacket(packet);
}

void Server::broadcastChatMessage(const std::string& sender, const std::string& message) {
    PacketWriter writer(PacketType::ChatMessage);
    writer.writeString(sender);
    writer.writeString(message);
    
    Packet packet;
    packet.type = PacketType::ChatMessage;
    packet.data = writer.build();
    broadcastPacket(packet);
    
    LOG_INFO("[Chat] {}: {}", sender, message);
}

void Server::broadcastSystemMessage(const std::string& message) {
    PacketWriter writer(PacketType::SystemMessage);
    writer.writeString(message);
    
    Packet packet;
    packet.type = PacketType::SystemMessage;
    packet.data = writer.build();
    broadcastPacket(packet);
    
    LOG_INFO("[System] {}", message);
}

} // namespace VoxelForge
