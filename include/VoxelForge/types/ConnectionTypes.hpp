#ifndef VOXELFORGE_CONNECTION_TYPES_HPP
#define VOXELFORGE_CONNECTION_TYPES_HPP

namespace VoxelForge {
enum class ConnectionState {
    Disconnected,
    Connecting,
    Handshaking,
    Connected,
    Disconnecting,
    Error
};
} // namespace VoxelForge

#endif // VOXELFORGE_CONNECTION_TYPES_HPP
