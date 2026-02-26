/**
 * @file ChunkSection.cpp
 * @brief Chunk section (16x16x16) implementation
 */

#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/world/BlockRegistry.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <cstring>

namespace VoxelForge {

// ============================================================================
// LightData Implementation
// ============================================================================

uint8_t LightData::getSkyLight(int x, int y, int z) const {
    int index = getIndex(x, y, z);
    return skyLight[index];
}

uint8_t LightData::getBlockLight(int x, int y, int z) const {
    int index = getIndex(x, y, z);
    return blockLight[index];
}

void LightData::setSkyLight(int x, int y, int z, uint8_t value) {
    int index = getIndex(x, y, z);
    skyLight[index] = value;
}

void LightData::setBlockLight(int x, int y, int z, uint8_t value) {
    int index = getIndex(x, y, z);
    blockLight[index] = value;
}

// ============================================================================
// BlockPalette Implementation
// ============================================================================

BlockPalette::BlockPalette() 
    : bitsPerBlock(SINGLE_VALUE_BITS)
    , isSingleValue(true) {
    palette.push_back(BlockState()); // Air as default
}

BlockState BlockPalette::get(int x, int y, int z) const {
    if (isSingleValue) {
        return singleValue;
    }
    
    int index = getIndex(x, y, z);
    
    // Extract value from packed data
    int bitIndex = index * bitsPerBlock;
    int arrayIndex = bitIndex / 64;
    int bitOffset = bitIndex % 64;
    
    if (arrayIndex >= static_cast<int>(data.size())) {
        return BlockState();
    }
    
    uint64_t value = data[arrayIndex] >> bitOffset;
    
    // Handle crossing array boundaries
    if (bitOffset + bitsPerBlock > 64 && arrayIndex + 1 < static_cast<int>(data.size())) {
        uint64_t nextValue = data[arrayIndex + 1];
        value |= (nextValue << (64 - bitOffset));
    }
    
    value &= (1ULL << bitsPerBlock) - 1;
    
    if (value < palette.size()) {
        return palette[value];
    }
    
    return BlockState();
}

bool BlockPalette::set(int x, int y, int z, BlockState state) {
    int index = getIndex(x, y, z);
    
    // Handle single-value optimization
    if (isSingleValue) {
        if (singleValue == state) {
            return false; // No change
        }
        
        // Transition to multi-value
        isSingleValue = false;
        bitsPerBlock = MIN_BITS;
        
        // Initialize data array
        int entriesPerLong = 64 / bitsPerBlock;
        int dataLength = (BLOCKS_PER_SECTION + entriesPerLong - 1) / entriesPerLong;
        data.resize(dataLength, 0);
        
        // Fill with single value
        if (!singleValue.isAir()) {
            palette.push_back(singleValue);
            for (int i = 0; i < BLOCKS_PER_SECTION; i++) {
                int bitIndex = i * bitsPerBlock;
                int arrayIndex = bitIndex / 64;
                int bitOffset = bitIndex % 64;
                data[arrayIndex] |= (1ULL << bitOffset);
            }
        }
    }
    
    // Find or add state to palette
    int paletteIndex = findInPalette(state);
    if (paletteIndex < 0) {
        paletteIndex = addToPalette(state);
    }
    
    // Check if we need to resize
    int maxIndex = (1 << bitsPerBlock) - 1;
    if (paletteIndex > maxIndex) {
        resize(bitsPerBlock + 1);
    }
    
    // Set value in packed data
    int bitIndex = index * bitsPerBlock;
    int arrayIndex = bitIndex / 64;
    int bitOffset = bitIndex % 64;
    
    uint64_t mask = (1ULL << bitsPerBlock) - 1;
    uint64_t value = static_cast<uint64_t>(paletteIndex) & mask;
    
    // Clear old value
    data[arrayIndex] &= ~(mask << bitOffset);
    
    // Set new value
    data[arrayIndex] |= (value << bitOffset);
    
    // Handle crossing array boundaries
    if (bitOffset + bitsPerBlock > 64 && arrayIndex + 1 < static_cast<int>(data.size())) {
        int overflowBits = bitOffset + bitsPerBlock - 64;
        data[arrayIndex + 1] &= ~(mask >> (bitsPerBlock - overflowBits));
        data[arrayIndex + 1] |= (value >> (bitsPerBlock - overflowBits));
    }
    
    return true;
}

int BlockPalette::findInPalette(BlockState state) const {
    for (size_t i = 0; i < palette.size(); i++) {
        if (palette[i] == state) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int BlockPalette::addToPalette(BlockState state) {
    int index = static_cast<int>(palette.size());
    palette.push_back(state);
    return index;
}

void BlockPalette::resize(int newBitsPerBlock) {
    if (newBitsPerBlock == bitsPerBlock) return;
    if (newBitsPerBlock > MAX_BITS) {
        newBitsPerBlock = MAX_BITS;
    }
    
    // Save old data
    auto oldData = data;
    auto oldPalette = palette;
    int oldBitsPerBlock = bitsPerBlock;
    
    // Create new data array
    bitsPerBlock = newBitsPerBlock;
    int entriesPerLong = 64 / bitsPerBlock;
    int dataLength = (BLOCKS_PER_SECTION + entriesPerLong - 1) / entriesPerLong;
    data.clear();
    data.resize(dataLength, 0);
    
    // Re-pack all values
    for (int i = 0; i < BLOCKS_PER_SECTION; i++) {
        // Extract old value
        int oldBitIndex = i * oldBitsPerBlock;
        int oldArrayIndex = oldBitIndex / 64;
        int oldBitOffset = oldBitIndex % 64;
        
        uint64_t oldValue = 0;
        if (oldArrayIndex < static_cast<int>(oldData.size())) {
            oldValue = oldData[oldArrayIndex] >> oldBitOffset;
            if (oldBitOffset + oldBitsPerBlock > 64 && oldArrayIndex + 1 < static_cast<int>(oldData.size())) {
                oldValue |= (oldData[oldArrayIndex + 1] << (64 - oldBitOffset));
            }
            oldValue &= (1ULL << oldBitsPerBlock) - 1;
        }
        
        // Insert new value
        int newBitIndex = i * bitsPerBlock;
        int newArrayIndex = newBitIndex / 64;
        int newBitOffset = newBitIndex % 64;
        
        uint64_t mask = (1ULL << bitsPerBlock) - 1;
        
        data[newArrayIndex] &= ~(mask << newBitOffset);
        data[newArrayIndex] |= ((oldValue & mask) << newBitOffset);
        
        if (newBitOffset + bitsPerBlock > 64 && newArrayIndex + 1 < static_cast<int>(data.size())) {
            int overflowBits = newBitOffset + bitsPerBlock - 64;
            data[newArrayIndex + 1] &= ~(mask >> (bitsPerBlock - overflowBits));
            data[newArrayIndex + 1] |= (oldValue >> (bitsPerBlock - overflowBits));
        }
    }
}

std::vector<uint8_t> BlockPalette::serialize() const {
    std::vector<uint8_t> result;
    
    // Bits per block
    result.push_back(static_cast<uint8_t>(bitsPerBlock));
    
    // Palette size
    uint32_t paletteSize = static_cast<uint32_t>(palette.size());
    result.insert(result.end(), 
                  reinterpret_cast<uint8_t*>(&paletteSize),
                  reinterpret_cast<uint8_t*>(&paletteSize) + sizeof(paletteSize));
    
    // Palette entries
    for (const auto& state : palette) {
        uint64_t encoded = state.encode();
        result.insert(result.end(),
                      reinterpret_cast<uint8_t*>(&encoded),
                      reinterpret_cast<uint8_t*>(&encoded) + sizeof(encoded));
    }
    
    // Data size
    uint32_t dataSize = static_cast<uint32_t>(data.size());
    result.insert(result.end(),
                  reinterpret_cast<uint8_t*>(&dataSize),
                  reinterpret_cast<uint8_t*>(&dataSize) + sizeof(dataSize));
    
    // Data
    for (uint64_t value : data) {
        result.insert(result.end(),
                      reinterpret_cast<uint8_t*>(&value),
                      reinterpret_cast<uint8_t*>(&value) + sizeof(value));
    }
    
    return result;
}

BlockPalette BlockPalette::deserialize(const uint8_t* data, size_t size) {
    BlockPalette palette;
    size_t offset = 0;
    
    if (size < 1) return palette;
    
    palette.bitsPerBlock = data[offset++];
    
    if (offset + sizeof(uint32_t) > size) return palette;
    uint32_t paletteSize;
    memcpy(&paletteSize, data + offset, sizeof(paletteSize));
    offset += sizeof(paletteSize);
    
    palette.palette.resize(paletteSize);
    for (uint32_t i = 0; i < paletteSize; i++) {
        if (offset + sizeof(uint64_t) > size) return palette;
        uint64_t encoded;
        memcpy(&encoded, data + offset, sizeof(encoded));
        offset += sizeof(encoded);
        palette.palette[i] = BlockState::decode(encoded);
    }
    
    if (offset + sizeof(uint32_t) > size) return palette;
    uint32_t dataSize;
    memcpy(&dataSize, data + offset, sizeof(dataSize));
    offset += sizeof(dataSize);
    
    palette.data.resize(dataSize);
    for (uint32_t i = 0; i < dataSize; i++) {
        if (offset + sizeof(uint64_t) > size) return palette;
        memcpy(&palette.data[i], data + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);
    }
    
    palette.isSingleValue = false;
    
    return palette;
}

// ============================================================================
// ChunkSection Implementation
// ============================================================================

ChunkSection::ChunkSection() {
    // Initialize light data to full brightness
    skyLight.fill(15);
    blockLight.fill(0);
}

BlockState ChunkSection::getBlock(int x, int y, int z) const {
    if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= SECTION_HEIGHT || 
        z < 0 || z >= CHUNK_WIDTH) {
        return BlockState();
    }
    return blocks.get(x, y, z);
}

void ChunkSection::setBlock(int x, int y, int z, BlockState state) {
    if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= SECTION_HEIGHT || 
        z < 0 || z >= CHUNK_WIDTH) {
        return;
    }
    
    BlockState oldState = blocks.get(x, y, z);
    
    if (oldState == state) {
        return; // No change
    }
    
    // Update non-air counter
    if (oldState.isAir() && !state.isAir()) {
        nonAirBlocks++;
    } else if (!oldState.isAir() && state.isAir()) {
        nonAirBlocks--;
    }
    
    blocks.set(x, y, z, state);
    dirty = true;
    meshDirty = true;
}

uint8_t ChunkSection::getSkyLight(int x, int y, int z) const {
    return lighting.getSkyLight(x, y, z);
}

uint8_t ChunkSection::getBlockLight(int x, int y, int z) const {
    return lighting.getBlockLight(x, y, z);
}

void ChunkSection::setSkyLight(int x, int y, int z, uint8_t value) {
    lighting.setSkyLight(x, y, z, value);
    dirty = true;
}

void ChunkSection::setBlockLight(int x, int y, int z, uint8_t value) {
    lighting.setBlockLight(x, y, z, value);
    dirty = true;
}

bool ChunkSection::hasBlockEntity(int x, int y, int z) const {
    return blockEntities.find(getBlockEntityIndex(x, y, z)) != blockEntities.end();
}

void ChunkSection::setBlockEntity(int x, int y, int z, std::unique_ptr<BlockEntity> entity) {
    blockEntities[getBlockEntityIndex(x, y, z)] = std::move(entity);
    dirty = true;
}

BlockEntity* ChunkSection::getBlockEntity(int x, int y, int z) const {
    auto it = blockEntities.find(getBlockEntityIndex(x, y, z));
    if (it != blockEntities.end()) {
        return it->second.get();
    }
    return nullptr;
}

void ChunkSection::removeBlockEntity(int x, int y, int z) {
    blockEntities.erase(getBlockEntityIndex(x, y, z));
    dirty = true;
}

std::vector<uint8_t> ChunkSection::serialize() const {
    std::vector<uint8_t> result;
    
    // Serialize block palette
    auto paletteData = blocks.serialize();
    uint32_t paletteSize = static_cast<uint32_t>(paletteData.size());
    result.insert(result.end(),
                  reinterpret_cast<uint8_t*>(&paletteSize),
                  reinterpret_cast<uint8_t*>(&paletteSize) + sizeof(paletteSize));
    result.insert(result.end(), paletteData.begin(), paletteData.end());
    
    // Serialize light data
    result.insert(result.end(), lighting.skyLight.begin(), lighting.skyLight.end());
    result.insert(result.end(), lighting.blockLight.begin(), lighting.blockLight.end());
    
    // Non-air block count
    int32_t count = nonAirBlocks.load();
    result.insert(result.end(),
                  reinterpret_cast<uint8_t*>(&count),
                  reinterpret_cast<uint8_t*>(&count) + sizeof(count));
    
    return result;
}

std::unique_ptr<ChunkSection> ChunkSection::deserialize(const uint8_t* data, size_t size) {
    auto section = std::make_unique<ChunkSection>();
    size_t offset = 0;
    
    // Read palette size
    if (offset + sizeof(uint32_t) > size) return section;
    uint32_t paletteSize;
    memcpy(&paletteSize, data + offset, sizeof(paletteSize));
    offset += sizeof(paletteSize);
    
    // Read palette data
    if (offset + paletteSize > size) return section;
    section->blocks = BlockPalette::deserialize(data + offset, paletteSize);
    offset += paletteSize;
    
    // Read light data
    if (offset + BLOCKS_PER_SECTION * 2 > size) return section;
    memcpy(section->lighting.skyLight.data(), data + offset, BLOCKS_PER_SECTION);
    offset += BLOCKS_PER_SECTION;
    memcpy(section->lighting.blockLight.data(), data + offset, BLOCKS_PER_SECTION);
    offset += BLOCKS_PER_SECTION;
    
    // Read non-air count
    if (offset + sizeof(int32_t) > size) return section;
    int32_t count;
    memcpy(&count, data + offset, sizeof(count));
    section->nonAirBlocks = count;
    
    return section;
}

// ============================================================================
// HeightMap Implementation
// ============================================================================

int HeightMap::getHeight(int x, int z) const {
    if (x < 0 || x >= CHUNK_WIDTH || z < 0 || z >= CHUNK_WIDTH) {
        return 0;
    }
    return data[getIndex(x, z)];
}

void HeightMap::setHeight(int x, int z, int height) {
    if (x < 0 || x >= CHUNK_WIDTH || z < 0 || z >= CHUNK_WIDTH) {
        return;
    }
    data[getIndex(x, z)] = static_cast<int16_t>(height);
}

void HeightMap::update(const ChunkSection* section, int sectionY, Type type) {
    int baseY = sectionY * SECTION_HEIGHT + CHUNK_MIN_Y;
    
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_WIDTH; z++) {
            // Find highest block in this column for this section
            int highestInSection = -1;
            
            for (int y = SECTION_HEIGHT - 1; y >= 0; y--) {
                BlockState state = section->getBlock(x, y, z);
                
                if (state.isAir()) continue;
                
                // Check if block matches heightmap type
                bool include = false;
                switch (type) {
                    case Type::WorldSurface:
                        include = true;
                        break;
                    case Type::MotionBlocking:
                        // Include solid blocks and water
                        include = state.isSolid() || 
                            state.getBlockId() == BlockRegistry::get().getBlockId("minecraft:water");
                        break;
                    case Type::OceanFloor:
                        // Include solid blocks
                        include = state.isSolid();
                        break;
                    case Type::MotionBlockingNoLeaves:
                        // Include solid blocks but not leaves
                        include = state.isSolid();
                        // TODO: Check if not leaves
                        break;
                }
                
                if (include) {
                    highestInSection = baseY + y;
                    break;
                }
            }
            
            // Update heightmap if this section is higher
            if (highestInSection >= 0) {
                int idx = getIndex(x, z);
                if (highestInSection > data[idx]) {
                    data[idx] = static_cast<int16_t>(highestInSection);
                }
            }
        }
    }
}

// ============================================================================
// BiomeStorage Implementation
// ============================================================================

BiomeStorage::BiomeStorage() {
    data.fill(1); // Default to plains biome
}

BiomeStorage::BiomeID BiomeStorage::get(int x, int y, int z) const {
    return data[getIndex(x, y, z)];
}

void BiomeStorage::set(int x, int y, int z, BiomeID biome) {
    data[getIndex(x, y, z)] = biome;
}

BiomeStorage::BiomeID BiomeStorage::getColumn(int x, int z) const {
    // Return biome from bottom section
    return get(x, 0, z);
}

void BiomeStorage::setColumn(int x, int z, BiomeID biome) {
    // Set same biome for entire column
    for (int y = 0; y < 4; y++) {
        set(x, y * 4, z, biome);
    }
}

} // namespace VoxelForge
