/**
 * @file ChunkSection.cpp
 * @brief Chunk section implementation
 */

#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <cstring>

namespace VoxelForge {

// ============================================
// Light Data
// ============================================

uint8_t LightData::getSkyLight(int x, int y, int z) const {
    int index = getIndex(x, y, z);
    return (skyLight[index / 2] >> (index % 2 * 4)) & 0x0F;
}

uint8_t LightData::getBlockLight(int x, int y, int z) const {
    int index = getIndex(x, y, z);
    return (blockLight[index / 2] >> (index % 2 * 4)) & 0x0F;
}

void LightData::setSkyLight(int x, int y, int z, uint8_t value) {
    int index = getIndex(x, y, z);
    int offset = index % 2 * 4;
    skyLight[index / 2] = (skyLight[index / 2] & ~(0x0F << offset)) | ((value & 0x0F) << offset);
}

void LightData::setBlockLight(int x, int y, int z, uint8_t value) {
    int index = getIndex(x, y, z);
    int offset = index % 2 * 4;
    blockLight[index / 2] = (blockLight[index / 2] & ~(0x0F << offset)) | ((value & 0x0F) << offset);
}

// ============================================
// Block Palette
// ============================================

BlockPalette::BlockPalette() 
    : bitsPerBlock(MIN_BITS), isSingleValue(true), singleValue(BlockState()) {
    data.push_back(0);
}

BlockState BlockPalette::get(int x, int y, int z) const {
    if (isSingleValue) {
        return singleValue;
    }
    
    int index = y * CHUNK_WIDTH * CHUNK_WIDTH + z * CHUNK_WIDTH + x;
    int blocksPerLong = 64 / bitsPerBlock;
    int longIndex = index / blocksPerLong;
    int bitOffset = (index % blocksPerLong) * bitsPerBlock;
    
    if (longIndex >= static_cast<int>(data.size())) {
        return BlockState();
    }
    
    uint64_t packed = (data[longIndex] >> bitOffset) & ((1ULL << bitsPerBlock) - 1);
    
    if (packed < palette.size()) {
        return palette[packed];
    }
    
    return BlockState();
}

bool BlockPalette::set(int x, int y, int z, BlockState state) {
    int index = y * CHUNK_WIDTH * CHUNK_WIDTH + z * CHUNK_WIDTH + x;
    
    // First block in empty section
    if (isSingleValue && singleValue.isAir()) {
        singleValue = state;
        isSingleValue = true;
        return true;
    }
    
    // Check if same as current single value
    if (isSingleValue && singleValue == state) {
        return false; // No change
    }
    
    // Need to expand to palette
    if (isSingleValue) {
        // Expand single value to palette
        palette.push_back(singleValue);
        palette.push_back(state);
        bitsPerBlock = MIN_BITS;
        isSingleValue = false;
        
        // Rebuild data
        data.clear();
        int blocksPerLong = 64 / bitsPerBlock;
        int longCount = (BLOCKS_PER_SECTION + blocksPerLong - 1) / blocksPerLong;
        data.resize(longCount, 0);
        
        // Set all blocks
        for (int i = 0; i < BLOCKS_PER_SECTION; i++) {
            int li = i / blocksPerLong;
            int bo = (i % blocksPerLong) * bitsPerBlock;
            data[li] |= (i == index ? 1ULL : 0ULL) << bo;
        }
        
        return true;
    }
    
    // Find or add to palette
    int paletteIndex = findInPalette(state);
    if (paletteIndex < 0) {
        paletteIndex = addToPalette(state);
        
        // Check if we need to resize
        if (bitsPerBlock < MAX_BITS && palette.size() > static_cast<size_t>(1 << bitsPerBlock)) {
            resize(bitsPerBlock + 1);
        }
    }
    
    // Set the block
    int blocksPerLong = 64 / bitsPerBlock;
    int longIndex = index / blocksPerLong;
    int bitOffset = (index % blocksPerLong) * bitsPerBlock;
    
    if (longIndex >= static_cast<int>(data.size())) {
        return false;
    }
    
    // Clear old value and set new
    uint64_t mask = ((1ULL << bitsPerBlock) - 1) << bitOffset;
    data[longIndex] = (data[longIndex] & ~mask) | (static_cast<uint64_t>(paletteIndex) << bitOffset);
    
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
    
    // Create new data array
    int newBlocksPerLong = 64 / newBitsPerBlock;
    int newLongCount = (BLOCKS_PER_SECTION + newBlocksPerLong - 1) / newBlocksPerLong;
    std::vector<uint64_t> newData(newLongCount, 0);
    
    // Copy blocks
    for (int i = 0; i < BLOCKS_PER_SECTION; i++) {
        // Get old value
        int oldBlocksPerLong = 64 / bitsPerBlock;
        int oldLongIndex = i / oldBlocksPerLong;
        int oldBitOffset = (i % oldBlocksPerLong) * bitsPerBlock;
        
        uint64_t value = (data[oldLongIndex] >> oldBitOffset) & ((1ULL << bitsPerBlock) - 1);
        
        // Set new value
        int newLongIndex = i / newBlocksPerLong;
        int newBitOffset = (i % newBlocksPerLong) * newBitsPerBlock;
        
        newData[newLongIndex] |= value << newBitOffset;
    }
    
    data = std::move(newData);
    bitsPerBlock = newBitsPerBlock;
}

// ============================================
// Chunk Section
// ============================================

ChunkSection::ChunkSection() {
}

BlockState ChunkSection::getBlock(int x, int y, int z) const {
    return blocks.get(x, y, z);
}

void ChunkSection::setBlock(int x, int y, int z, BlockState state) {
    BlockState old = getBlock(x, y, z);
    
    if (state.isAir() && !old.isAir()) {
        nonAirBlocks--;
    } else if (!state.isAir() && old.isAir()) {
        nonAirBlocks++;
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

// ============================================
// Height Map
// ============================================

int HeightMap::getHeight(int x, int z) const {
    return data[getIndex(x, z)];
}

void HeightMap::setHeight(int x, int z, int height) {
    data[getIndex(x, z)] = static_cast<int16_t>(height);
}

// ============================================
// Biome Storage
// ============================================

BiomeStorage::BiomeStorage() {
    std::fill(data.begin(), data.end(), 0);
}

BiomeStorage::BiomeID BiomeStorage::get(int x, int y, int z) const {
    return data[getIndex(x, y, z)];
}

void BiomeStorage::set(int x, int y, int z, BiomeID biome) {
    data[getIndex(x, y, z)] = biome;
}

BiomeStorage::BiomeID BiomeStorage::getColumn(int x, int z) const {
    return get(x, 0, z);
}

void BiomeStorage::setColumn(int x, int z, BiomeID biome) {
    for (int y = 0; y < 16; y++) {
        set(x, y, z, biome);
    }
}

// ============================================
// Chunk
// ============================================

Chunk::Chunk(const ChunkPos& pos) : position(pos) {
    // Initialize all sections
    for (int i = 0; i < SECTIONS_PER_CHUNK; i++) {
        sections[i] = std::make_unique<ChunkSection>();
    }
}

Chunk::~Chunk() {
}

BlockState Chunk::getBlock(int x, int y, int z) const {
    if (!isValidY(y) || !isValidXZ(x, z)) {
        return BlockState();
    }
    
    int sectionY = getSectionIndex(y);
    if (hasSection(sectionY)) {
        int localY = y - sectionY * SECTION_HEIGHT - CHUNK_MIN_Y;
        return sections[sectionY]->getBlock(x, localY, z);
    }
    
    return BlockState();
}

void Chunk::setBlock(int x, int y, int z, BlockState state) {
    if (!isValidY(y) || !isValidXZ(x, z)) {
        return;
    }
    
    int sectionY = getSectionIndex(y);
    if (hasSection(sectionY)) {
        int localY = y - sectionY * SECTION_HEIGHT - CHUNK_MIN_Y;
        sections[sectionY]->setBlock(x, localY, z, state);
        dirty = true;
    }
}

ChunkSection* Chunk::getSection(int sectionY) {
    if (sectionY >= 0 && sectionY < SECTIONS_PER_CHUNK) {
        return sections[sectionY].get();
    }
    return nullptr;
}

const ChunkSection* Chunk::getSection(int sectionY) const {
    if (sectionY >= 0 && sectionY < SECTIONS_PER_CHUNK) {
        return sections[sectionY].get();
    }
    return nullptr;
}

bool Chunk::hasSection(int sectionY) const {
    return sectionY >= 0 && sectionY < SECTIONS_PER_CHUNK && sections[sectionY] != nullptr;
}

uint8_t Chunk::getSkyLight(int x, int y, int z) const {
    if (!isValidY(y) || !isValidXZ(x, z)) {
        return 15;
    }
    
    int sectionY = getSectionIndex(y);
    if (hasSection(sectionY)) {
        int localY = y - sectionY * SECTION_HEIGHT - CHUNK_MIN_Y;
        return sections[sectionY]->getSkyLight(x, localY, z);
    }
    
    return 15;
}

uint8_t Chunk::getBlockLight(int x, int y, int z) const {
    if (!isValidY(y) || !isValidXZ(x, z)) {
        return 0;
    }
    
    int sectionY = getSectionIndex(y);
    if (hasSection(sectionY)) {
        int localY = y - sectionY * SECTION_HEIGHT - CHUNK_MIN_Y;
        return sections[sectionY]->getBlockLight(x, localY, z);
    }
    
    return 0;
}

void Chunk::setSkyLight(int x, int y, int z, uint8_t value) {
    if (!isValidY(y) || !isValidXZ(x, z)) {
        return;
    }
    
    int sectionY = getSectionIndex(y);
    if (hasSection(sectionY)) {
        int localY = y - sectionY * SECTION_HEIGHT - CHUNK_MIN_Y;
        sections[sectionY]->setSkyLight(x, localY, z, value);
    }
}

void Chunk::setBlockLight(int x, int y, int z, uint8_t value) {
    if (!isValidY(y) || !isValidXZ(x, z)) {
        return;
    }
    
    int sectionY = getSectionIndex(y);
    if (hasSection(sectionY)) {
        int localY = y - sectionY * SECTION_HEIGHT - CHUNK_MIN_Y;
        sections[sectionY]->setBlockLight(x, localY, z, value);
    }
}

BlockEntity* Chunk::getBlockEntity(const BlockPos& pos) {
    int sectionY = getSectionIndex(pos.y);
    if (hasSection(sectionY)) {
        int localY = pos.y - sectionY * SECTION_HEIGHT - CHUNK_MIN_Y;
        return sections[sectionY]->getBlockEntity(pos.x, localY, pos.z);
    }
    return nullptr;
}

void Chunk::setBlockEntity(const BlockPos& pos, std::unique_ptr<BlockEntity> entity) {
    int sectionY = getSectionIndex(pos.y);
    if (hasSection(sectionY)) {
        int localY = pos.y - sectionY * SECTION_HEIGHT - CHUNK_MIN_Y;
        sections[sectionY]->setBlockEntity(pos.x, localY, pos.z, std::move(entity));
    }
}

void Chunk::removeBlockEntity(const BlockPos& pos) {
    int sectionY = getSectionIndex(pos.y);
    if (hasSection(sectionY)) {
        int localY = pos.y - sectionY * SECTION_HEIGHT - CHUNK_MIN_Y;
        sections[sectionY]->removeBlockEntity(pos.x, localY, pos.z);
    }
}

const std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>>& Chunk::getBlockEntities() const {
    // This would need to aggregate from all sections
    static std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> empty;
    return empty;
}

int Chunk::getHeight(HeightMap::Type type, int x, int z) const {
    switch (type) {
        case HeightMap::Type::WorldSurface:
            return worldSurfaceHeightMap.getHeight(x, z);
        case HeightMap::Type::MotionBlocking:
            return motionBlockingHeightMap.getHeight(x, z);
        default:
            return 0;
    }
}

void Chunk::recalculateHeightMaps() {
    // Calculate from top to bottom
    for (int x = 0; x < CHUNK_WIDTH; x++) {
        for (int z = 0; z < CHUNK_WIDTH; z++) {
            int surfaceHeight = CHUNK_MIN_Y;
            int motionBlockingHeight = CHUNK_MIN_Y;
            
            for (int y = CHUNK_MIN_Y + CHUNK_HEIGHT - 1; y >= CHUNK_MIN_Y; y--) {
                BlockState state = getBlock(x, y, z);
                if (!state.isAir()) {
                    if (surfaceHeight == CHUNK_MIN_Y) {
                        surfaceHeight = y;
                    }
                    if (state.isSolid() && motionBlockingHeight == CHUNK_MIN_Y) {
                        motionBlockingHeight = y;
                    }
                    if (surfaceHeight != CHUNK_MIN_Y && motionBlockingHeight != CHUNK_MIN_Y) {
                        break;
                    }
                }
            }
            
            worldSurfaceHeightMap.setHeight(x, z, surfaceHeight);
            motionBlockingHeightMap.setHeight(x, z, motionBlockingHeight);
        }
    }
}

BiomeStorage::BiomeID Chunk::getBiome(int x, int y, int z) const {
    return biomes.get(x, y, z);
}

void Chunk::setBiome(int x, int y, int z, BiomeStorage::BiomeID biome) {
    biomes.set(x, y, z, biome);
}

bool Chunk::isDirty() const {
    if (dirty) return true;
    for (const auto& section : sections) {
        if (section && section->isDirty()) return true;
    }
    return false;
}

void Chunk::markDirty() {
    dirty = true;
}

void Chunk::markClean() {
    dirty = false;
    for (auto& section : sections) {
        if (section) {
            section->markClean();
        }
    }
}

} // namespace VoxelForge
