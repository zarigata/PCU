/**
 * @file AnvilLoader.cpp
 * @brief Minecraft Anvil world format loader
 */

#include <VoxelForge/world/AnvilLoader.hpp>
#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/utils/NBT.hpp>
#include <VoxelForge/utils/Compression.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <fstream>

namespace VoxelForge {

AnvilLoader::AnvilLoader(const std::string& worldPath) : worldPath_(worldPath) {
    LOG_INFO("AnvilLoader created for world: {}", worldPath);
}

std::unique_ptr<Chunk> AnvilLoader::loadChunk(const ChunkPos& pos) {
    // Determine region file
    int regionX = pos.x >> 5;  // Divide by 32
    int regionZ = pos.z >> 5;
    
    std::string regionFile = worldPath_ + "/region/r." + 
        std::to_string(regionX) + "." + std::to_string(regionZ) + ".mca";
    
    // Check if file exists
    std::ifstream file(regionFile, std::ios::binary);
    if (!file.is_open()) {
        LOG_DEBUG("Region file not found: {}", regionFile);
        return nullptr;
    }
    
    // Read chunk location in region file
    int localX = pos.x & 31;
    int localZ = pos.z & 31;
    int locationOffset = 4 * (localX + localZ * 32);
    
    file.seekg(locationOffset);
    uint32_t location;
    file.read(reinterpret_cast<char*>(&location), 4);
    
    // Convert from big-endian
    location = ((location & 0xFF) << 24) | 
               (((location >> 8) & 0xFF) << 16) |
               (((location >> 16) & 0xFF) << 8) |
               ((location >> 24) & 0xFF);
    
    if (location == 0) {
        // Chunk not generated
        return nullptr;
    }
    
    // Extract sector offset and size
    uint32_t sectorOffset = (location >> 8) * 4096;
    // uint8_t sectorCount = location & 0xFF;
    
    // Read chunk data
    file.seekg(sectorOffset);
    uint32_t length;
    file.read(reinterpret_cast<char*>(&length), 4);
    length = ((length & 0xFF) << 24) | (((length >> 8) & 0xFF) << 16) |
             (((length >> 16) & 0xFF) << 8) | ((length >> 24) & 0xFF);
    
    uint8_t compressionType;
    file.read(reinterpret_cast<char*>(&compressionType), 1);
    
    std::vector<uint8_t> compressedData(length - 1);
    file.read(reinterpret_cast<char*>(compressedData.data()), length - 1);
    
    file.close();
    
    // Decompress data
    std::vector<uint8_t> decompressedData;
    if (compressionType == 1) {
        // GZIP compression
        decompressedData = Compression::decompressGzip(compressedData);
    } else if (compressionType == 2) {
        // Zlib compression
        decompressedData = Compression::decompressZlib(compressedData);
    } else {
        LOG_ERROR("Unknown compression type: {}", compressionType);
        return nullptr;
    }
    
    // Parse NBT
    NBTCompound nbt = NBTCompound::parse(decompressedData.data(), decompressedData.size());
    
    // Create chunk from NBT
    return parseChunkNBT(nbt, pos);
}

bool AnvilLoader::saveChunk(const Chunk* chunk) {
    if (!chunk) return false;
    
    // TODO: Implement chunk saving
    LOG_WARN("Chunk saving not yet implemented");
    return false;
}

std::unique_ptr<Chunk> AnvilLoader::parseChunkNBT(const NBTCompound& nbt, const ChunkPos& pos) {
    auto chunk = std::make_unique<Chunk>(pos);
    
    // Parse level data
    if (nbt.has("Level")) {
        const auto& level = nbt.getCompound("Level");
        
        // Parse sections
        if (level.has("Sections")) {
            const auto& sections = level.getList("Sections");
            for (const auto& sectionNbt : sections) {
                parseSectionNBT(chunk.get(), sectionNbt);
            }
        }
        
        // Parse block entities
        if (level.has("TileEntities")) {
            const auto& tileEntities = level.getList("TileEntities");
            for (const auto& te : tileEntities) {
                // Parse block entity
            }
        }
        
        // Parse biomes
        if (level.has("Biomes")) {
            const auto& biomes = level.getByteArray("Biomes");
            // Set biome data
        }
    }
    
    return chunk;
}

void AnvilLoader::parseSectionNBT(Chunk* chunk, const NBTCompound& sectionNbt) {
    int sectionY = sectionNbt.getByte("Y");
    
    if (sectionY < 0 || sectionY >= SECTIONS_PER_CHUNK) return;
    
    // Parse block states
    if (sectionNbt.has("block_states")) {
        // 1.18+ format
        const auto& blockStates = sectionNbt.getCompound("block_states");
        // Parse palette and data
    } else if (sectionNbt.has("Palette") && sectionNbt.has("BlockStates")) {
        // Pre-1.18 format
        const auto& palette = sectionNbt.getList("Palette");
        const auto& blockStates = sectionNbt.getLongArray("BlockStates");
        // Parse and convert
    }
    
    // Parse light
    if (sectionNbt.has("SkyLight")) {
        const auto& skyLight = sectionNbt.getByteArray("SkyLight");
        // Set sky light
    }
    
    if (sectionNbt.has("BlockLight")) {
        const auto& blockLight = sectionNbt.getByteArray("BlockLight");
        // Set block light
    }
}

} // namespace VoxelForge
