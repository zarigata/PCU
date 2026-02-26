/**
 * @file Connection.cpp
 * @brief Connection implementation
 */

#include <VoxelForge/networking/Connection.hpp>
#include <VoxelForge/networking/NetworkManager.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {

// ============================================================================
// Connection Implementation
// ============================================================================

Connection::Connection() {
    lastRateUpdate = std::chrono::steady_clock::now();
    lastKeepAliveSent = std::chrono::steady_clock::now();
    lastKeepAliveReceived = std::chrono::steady_clock::now();
}

Connection::~Connection() {
    forceDisconnect();
}

bool Connection::connect(ENetHost* host, const std::string& address, uint16_t port) {
    if (peer) {
        LOG_ERROR("Already connected");
        return false;
    }
    
    ENetAddress addr;
    enet_address_set_host(&addr, address.c_str());
    addr.port = port;
    
    peer = enet_host_connect(host, &addr, 4, 0);
    if (!peer) {
        LOG_ERROR("Failed to initiate connection to {}:{}", address, port);
        return false;
    }
    
    this->address = address;
    this->port = port;
    state = ConnectionState::Connecting;
    
    // Wait for connection with timeout
    ENetEvent event;
    if (enet_host_service(host, &event, config.timeout) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {
        peer = event.peer;
        peerId = nextClientId++;
        state = ConnectionState::Handshaking;
        stats.lastPacketTime = std::chrono::steady_clock::now();
        LOG_INFO("Connected to {}:{}", address, port);
        return true;
    }
    
    LOG_ERROR("Connection to {}:{} timed out", address, port);
    enet_peer_reset(peer);
    peer = nullptr;
    state = ConnectionState::Disconnected;
    return false;
}

void Connection::disconnect() {
    if (!peer) return;
    
    state = ConnectionState::Disconnecting;
    
    enet_peer_disconnect(peer, 0);
    
    // Wait for disconnect acknowledgment
    ENetEvent event;
    auto startTime = std::chrono::steady_clock::now();
    while (enet_host_service(enet_peer_get_host(peer), &event, 1000) > 0) {
        if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            break;
        }
        // Timeout check
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed > config.timeout) {
            break;
        }
    }
    
    enet_peer_reset(peer);
    peer = nullptr;
    state = ConnectionState::Disconnected;
    
    LOG_INFO("Disconnected from {}:{}", address, port);
}

void Connection::disconnect(const std::string& reason) {
    // Send disconnect reason
    PacketWriter writer(PacketType::Disconnect);
    writer.writeString(reason);
    
    Packet packet;
    packet.type = PacketType::Disconnect;
    packet.data = writer.build();
    packet.reliability = PacketReliability::Reliable;
    sendPacket(packet);
    
    disconnect();
    
    if (eventCallback) {
        ConnectionEvent event;
        event.type = ConnectionEvent::Type::Disconnected;
        event.message = reason;
        eventCallback(event);
    }
}

void Connection::forceDisconnect() {
    if (peer) {
        enet_peer_reset(peer);
        peer = nullptr;
    }
    state = ConnectionState::Disconnected;
}

void Connection::sendPacket(const Packet& packet) {
    if (!peer) return;
    
    uint32_t flags = 0;
    if (packet.reliability == PacketReliability::Reliable ||
        packet.reliability == PacketReliability::ReliableOrdered) {
        flags = ENET_PACKET_FLAG_RELIABLE;
    }
    
    ENetPacket* enetPacket = enet_packet_create(packet.data.data(),
                                                 packet.data.size(), flags);
    if (enetPacket && peer) {
        enet_peer_send(peer, packet.channel, enetPacket);
        stats.packetsSent++;
        stats.bytesSent += packet.data.size();
    }
}

void Connection::sendPacketImmediate(const Packet& packet) {
    sendPacket(packet);
    if (peer) {
        enet_host_flush(enet_peer_get_host(peer));
    }
}

void Connection::sendPacketReliable(const Packet& packet) {
    Packet p = packet;
    p.reliability = PacketReliability::Reliable;
    sendPacket(p);
}

void Connection::sendPacketUnreliable(const Packet& packet) {
    Packet p = packet;
    p.reliability = PacketReliability::Unreliable;
    sendPacket(p);
}

std::unique_ptr<Packet> Connection::receivePacket() {
    if (incomingPackets.empty()) return nullptr;
    
    auto packet = std::move(incomingPackets.front());
    incomingPackets.pop();
    return packet;
}

void Connection::update() {
    if (!peer) return;
    
    updateRates();
    checkTimeout();
    
    // Send keep-alive if needed
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastKeepAliveSent).count();
    
    if (elapsed > config.keepAliveInterval) {
        sendKeepAlive();
    }
}

void Connection::processEvents() {
    if (!peer) return;
    
    ENetHost* host = enet_peer_get_host(peer);
    ENetEvent event;
    
    while (enet_host_service(host, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE: {
                if (event.packet && event.packet->dataLength > 0) {
                    auto packet = std::make_unique<Packet>();
                    packet->type = static_cast<PacketType>(event.packet->data[0]);
                    packet->data.assign(event.packet->data + 1,
                                       event.packet->data + event.packet->dataLength);
                    packet->clientId = peerId;
                    
                    stats.packetsReceived++;
                    stats.bytesReceived += event.packet->dataLength;
                    stats.lastPacketTime = std::chrono::steady_clock::now();
                    
                    // Handle keep-alive
                    if (packet->type == PacketType::KeepAlive) {
                        lastKeepAliveReceived = std::chrono::steady_clock::now();
                    } else {
                        incomingPackets.push(std::move(packet));
                    }
                    
                    if (eventCallback) {
                        ConnectionEvent ce;
                        ce.type = ConnectionEvent::Type::PacketReceived;
                        eventCallback(ce);
                    }
                }
                enet_packet_destroy(event.packet);
                break;
            }
            
            case ENET_EVENT_TYPE_DISCONNECT: {
                state = ConnectionState::Disconnected;
                peer = nullptr;
                
                if (eventCallback) {
                    ConnectionEvent ce;
                    ce.type = ConnectionEvent::Type::Disconnected;
                    ce.message = "Connection closed by remote host";
                    eventCallback(ce);
                }
                break;
            }
            
            default:
                break;
        }
    }
}

void Connection::resetStats() {
    stats = ConnectionStats{};
}

void Connection::sendKeepAlive() {
    PacketWriter writer(PacketType::KeepAlive);
    writer.writeU64(std::chrono::steady_clock::now().time_since_epoch().count());
    
    Packet packet;
    packet.type = PacketType::KeepAlive;
    packet.data = writer.build();
    packet.reliability = PacketReliability::Unreliable;
    sendPacket(packet);
    
    lastKeepAliveSent = std::chrono::steady_clock::now();
}

void Connection::checkTimeout() {
    if (!peer) return;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - stats.lastPacketTime).count();
    
    if (elapsed > config.timeout) {
        if (eventCallback) {
            ConnectionEvent event;
            event.type = ConnectionEvent::Type::Timeout;
            event.message = "Connection timed out";
            eventCallback(event);
        }
        
        forceDisconnect();
    }
}

void Connection::updateStats() {
    if (!peer) return;
    
    // Calculate ping from round-trip time
    stats.pingMs = peer->roundTripTime;
}

void Connection::updateRates() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastRateUpdate).count();
    
    if (elapsed >= 1) {
        stats.uploadRate = static_cast<float>(stats.bytesSent - lastBytesSent) / elapsed;
        stats.downloadRate = static_cast<float>(stats.bytesReceived - lastBytesReceived) / elapsed;
        
        lastBytesSent = stats.bytesSent;
        lastBytesReceived = stats.bytesReceived;
        lastRateUpdate = now;
    }
}

// ============================================================================
// ConnectionFactory Implementation
// ============================================================================

std::unique_ptr<Connection> ConnectionFactory::create() {
    return std::make_unique<Connection>();
}

std::unique_ptr<Connection> ConnectionFactory::createAndConnect(ENetHost* host,
                                                                  const std::string& address,
                                                                  uint16_t port) {
    auto conn = create();
    if (conn->connect(host, address, port)) {
        return conn;
    }
    return nullptr;
}

// Static member initialization
uint32_t Connection::nextClientId = 1;

} // namespace VoxelForge
