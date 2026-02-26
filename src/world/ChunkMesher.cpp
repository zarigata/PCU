/**
 * @file ChunkMesher.cpp
 * @brief Chunk mesh generation implementation
 */

#include <VoxelForge/world/ChunkMesher.hpp>
#include <VoxelForge/world/World.hpp>
#include <VoxelForge/world/BlockRegistry.hpp>
#include <VoxelForge/rendering/VulkanBuffer.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <VoxelForge/core/Timer.hpp>
#include <glm/gtc/packing.hpp>
#include <algorithm>
#include <chrono>

namespace VoxelForge {

// ============================================================================
// Static Data - Face Geometry
// ============================================================================

// Face normals (right, left, up, down, front, back)
const glm::vec3 ChunkMesher::FACE_NORMALS[6] = {
    { 1.0f,  0.0f,  0.0f},  // Right (+X)
    {-1.0f,  0.0f,  0.0f},  // Left (-X)
    { 0.0f,  1.0f,  0.0f},  // Up (+Y)
    { 0.0f, -1.0f,  0.0f},  // Down (-Y)
    { 0.0f,  0.0f,  1.0f},  // Front (+Z)
    { 0.0f,  0.0f, -1.0f}   // Back (-Z)
};

// Face offsets for neighbor checking
const glm::ivec3 ChunkMesher::FACE_OFFSETS[6] = {
    { 1,  0,  0},  // Right
    {-1,  0,  0},  // Left
    { 0,  1,  0},  // Up
    { 0, -1,  0},  // Down
    { 0,  0,  1},  // Front
    { 0,  0, -1}   // Back
};

// Face vertex positions (local to the block corner)
// Each face has 4 vertices forming a quad
const int ChunkMesher::FACE_VERTICES[6][4][3] = {
    // Right face (+X)
    {{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}},
    // Left face (-X)
    {{0, 0, 1}, {0, 1, 1}, {0, 1, 0}, {0, 0, 0}},
    // Up face (+Y)
    {{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}},
    // Down face (-Y)
    {{0, 0, 1}, {0, 0, 0}, {1, 0, 0}, {1, 0, 1}},
    // Front face (+Z)
    {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}},
    // Back face (-Z)
    {{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}}
};

// Standard UV coordinates for a quad
const glm::vec2 ChunkMesher::FACE_UVS[4] = {
    {0.0f, 0.0f},
    {1.0f, 0.0f},
    {1.0f, 1.0f},
    {0.0f, 1.0f}
};

// ============================================================================
// ChunkMesher Implementation
// ============================================================================

ChunkMesher::ChunkMesher() {
    LOG_INFO("ChunkMesher created");
}

ChunkMesher::~ChunkMesher() {
    LOG_INFO("ChunkMesher destroyed");
}

void ChunkMesher::resetStats() {
    stats = Stats{};
}

ChunkMeshResult ChunkMesher::generateMesh(const Chunk* chunk, World* world) {
    ChunkMeshResult result;
    result.position = chunk->getPosition();
    
    if (!chunk) {
        LOG_ERROR("Null chunk passed to generateMesh");
        return result;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Generate mesh for each section
    for (int sectionY = 0; sectionY < SECTIONS_PER_CHUNK; sectionY++) {
        const ChunkSection* section = chunk->getSection(sectionY);
        
        if (!section || section->isEmpty()) {
            continue;
        }
        
        // Generate faces for all blocks in this section
        int baseY = CHUNK_MIN_Y + sectionY * SECTION_HEIGHT;
        
        for (int y = 0; y < SECTION_HEIGHT; y++) {
            int worldY = baseY + y;
            
            for (int z = 0; z < CHUNK_WIDTH; z++) {
                for (int x = 0; x < CHUNK_WIDTH; x++) {
                    BlockState blockState = section->getBlock(x, y, z);
                    
                    if (blockState.isAir()) {
                        continue;
                    }
                    
                    addBlockFaces(result.meshData, chunk, x, worldY, z, blockState, world);
                }
            }
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.generationTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    result.success = true;
    
    // Update statistics
    stats.meshesGenerated++;
    stats.totalVerticesGenerated += result.meshData.getTotalVertexCount();
    stats.totalIndicesGenerated += result.meshData.getTotalIndexCount();
    stats.totalTimeMs += result.generationTimeMs;
    stats.averageTimeMs = stats.totalTimeMs / stats.meshesGenerated;
    
    return result;
}

ChunkMeshResult ChunkMesher::generateSectionMesh(const Chunk* chunk, int sectionY, World* world) {
    ChunkMeshResult result;
    result.position = chunk->getPosition();
    
    if (!chunk || sectionY < 0 || sectionY >= SECTIONS_PER_CHUNK) {
        LOG_ERROR("Invalid parameters for generateSectionMesh");
        return result;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    const ChunkSection* section = chunk->getSection(sectionY);
    
    if (!section || section->isEmpty()) {
        result.success = true;  // Empty section is still "successful"
        return result;
    }
    
    int baseY = CHUNK_MIN_Y + sectionY * SECTION_HEIGHT;
    
    for (int y = 0; y < SECTION_HEIGHT; y++) {
        int worldY = baseY + y;
        
        for (int z = 0; z < CHUNK_WIDTH; z++) {
            for (int x = 0; x < CHUNK_WIDTH; x++) {
                BlockState blockState = section->getBlock(x, y, z);
                
                if (blockState.isAir()) {
                    continue;
                }
                
                addBlockFaces(result.meshData, chunk, x, worldY, z, blockState, world);
            }
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.generationTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    result.success = true;
    
    return result;
}

void ChunkMesher::addBlockFaces(
    ChunkMeshData& mesh,
    const Chunk* chunk,
    int localX, int localY, int localZ,
    BlockState blockState,
    World* world) {
    
    const auto& blockDef = BlockRegistry::get().getBlock(blockDef.getBlockId(blockState));
    if (!blockDef) return;
    
    glm::ivec3 blockPos(localX + chunk->getPosition().x * CHUNK_WIDTH,
                        localY,
                        localZ + chunk->getPosition().z * CHUNK_WIDTH);
    
    // Check each face
    for (int face = 0; face < 6; face++) {
        const auto& offset = FACE_OFFSETS[face];
        
        // Get neighbor block
        BlockState neighborState = getNeighborBlock(
            chunk, localX, localY, localZ,
            offset.x, offset.y, offset.z, world);
        
        // Check if we should render this face
        if (!config.cullHiddenFaces || shouldRenderFace(blockState, neighborState, face)) {
            // Calculate ambient occlusion for each vertex
            auto vertexAO = calculateVertexAO(chunk, localX, localY, localZ, face, world);
            
            // Calculate light for each vertex
            auto vertexLight = calculateVertexLight(chunk, localX, localY, localZ, face, world);
            
            // Add the face
            addFace(mesh, blockState, blockPos, face, vertexAO, vertexLight, world);
        }
    }
}

void ChunkMesher::addFace(
    ChunkMeshData& mesh,
    BlockState blockState,
    const glm::ivec3& position,
    int face,
    const std::array<uint8_t, 4>& vertexAO,
    const std::array<uint32_t, 4>& vertexLight,
    World* world) {
    
    const auto& blockDef = BlockRegistry::get().getBlock(blockDef.getBlockId(blockState));
    if (!blockDef) return;
    
    BlockRenderType renderType = blockDef->renderType;
    
    // Get texture coordinates
    glm::vec4 texCoords = getTextureCoords(blockState, face);
    float u0 = texCoords.x, v0 = texCoords.y;
    float u1 = texCoords.z, v1 = texCoords.w;
    
    // Get block color (for grass, leaves, etc.)
    uint32_t color = 0xFFFFFFFF; // Default white
    // TODO: Get actual block color based on biome
    
    // Calculate vertex positions
    glm::vec3 positions[4];
    for (int i = 0; i < 4; i++) {
        positions[i] = glm::vec3(
            position.x + FACE_VERTICES[face][i][0],
            position.y + FACE_VERTICES[face][i][1],
            position.z + FACE_VERTICES[face][i][2]
        );
    }
    
    // UV coordinates
    glm::vec2 uvs[4] = {
        {u0, v0},
        {u1, v0},
        {u1, v1},
        {u0, v1}
    };
    
    // Add the quad
    addQuad(mesh, positions, FACE_NORMALS[face], uvs, vertexAO, vertexLight, color, renderType);
}

bool ChunkMesher::shouldRenderFace(
    BlockState currentBlock,
    BlockState neighborBlock,
    int face) const {
    
    // Always render if neighbor is air
    if (neighborBlock.isAir()) {
        return true;
    }
    
    const auto& currentDef = BlockRegistry::get().getBlock(blockDef.getBlockId(currentBlock));
    const auto& neighborDef = BlockRegistry::get().getBlock(blockDef.getBlockId(neighborBlock));
    
    if (!currentDef || !neighborDef) {
        return true;
    }
    
    // Never render if same block (for solid blocks)
    if (currentBlock == neighborBlock && currentDef->renderType == BlockRenderType::Solid) {
        return false;
    }
    
    // Always render face if current block is transparent and neighbor is solid
    if (currentDef->renderType != BlockRenderType::Solid && 
        neighborDef->renderType == BlockRenderType::Solid) {
        return true;
    }
    
    // Don't render face between two solid blocks
    if (currentDef->renderType == BlockRenderType::Solid && 
        neighborDef->renderType == BlockRenderType::Solid) {
        return false;
    }
    
    // Don't render face if neighbor is opaque and not see-through
    if (!neighborDef->isSeeThrough) {
        return false;
    }
    
    return true;
}

BlockState ChunkMesher::getNeighborBlock(
    const Chunk* chunk,
    int localX, int localY, int localZ,
    int dx, int dy, int dz,
    World* world) const {
    
    int newX = localX + dx;
    int newY = localY + dy;
    int newZ = localZ + dz;
    
    // Check if within chunk bounds
    if (newX >= 0 && newX < CHUNK_WIDTH &&
        newZ >= 0 && newZ < CHUNK_WIDTH &&
        newY >= CHUNK_MIN_Y && newY < CHUNK_MIN_Y + CHUNK_HEIGHT) {
        
        return chunk->getBlock(newX, newY, newZ);
    }
    
    // Need to get from neighbor chunk or world
    if (world) {
        int worldX = chunk->getPosition().x * CHUNK_WIDTH + newX;
        int worldZ = chunk->getPosition().z * CHUNK_WIDTH + newZ;
        return world->getBlock(worldX, newY, worldZ);
    }
    
    return BlockState(); // Air
}

std::array<uint8_t, 4> ChunkMesher::calculateVertexAO(
    const Chunk* chunk,
    int x, int y, int z,
    int face,
    World* world) const {
    
    if (!config.enableAO) {
        return {0, 0, 0, 0};
    }
    
    std::array<uint8_t, 4> ao;
    
    // For each vertex of the face, check the 3 neighboring blocks
    // that affect its ambient occlusion
    
    // Vertex corner offsets based on face
    static const int vertexOffsets[6][4][3][3] = {
        // Right face
        {{{0,0,-1}, {0,-1,0}, {0,-1,-1}},
         {{0,0,-1}, {0,1,0}, {0,1,-1}},
         {{0,0,1}, {0,1,0}, {0,1,1}},
         {{0,0,1}, {0,-1,0}, {0,-1,1}}},
        // Left face
        {{{0,0,1}, {0,-1,0}, {0,-1,1}},
         {{0,0,1}, {0,1,0}, {0,1,1}},
         {{0,0,-1}, {0,1,0}, {0,1,-1}},
         {{0,0,-1}, {0,-1,0}, {0,-1,-1}}},
        // Up face
        {{{-1,0,0}, {0,0,1}, {-1,0,1}},
         {{1,0,0}, {0,0,1}, {1,0,1}},
         {{1,0,0}, {0,0,-1}, {1,0,-1}},
         {{-1,0,0}, {0,0,-1}, {-1,0,-1}}},
        // Down face
        {{{-1,0,0}, {0,0,-1}, {-1,0,-1}},
         {{1,0,0}, {0,0,-1}, {1,0,-1}},
         {{1,0,0}, {0,0,1}, {1,0,1}},
         {{-1,0,0}, {0,0,1}, {-1,0,1}}},
        // Front face
        {{{-1,0,0}, {0,-1,0}, {-1,-1,0}},
         {{1,0,0}, {0,-1,0}, {1,-1,0}},
         {{1,0,0}, {0,1,0}, {1,1,0}},
         {{-1,0,0}, {0,1,0}, {-1,1,0}}},
        // Back face
        {{{1,0,0}, {0,-1,0}, {1,-1,0}},
         {{-1,0,0}, {0,-1,0}, {-1,-1,0}},
         {{-1,0,0}, {0,1,0}, {-1,1,0}},
         {{1,0,0}, {0,1,0}, {1,1,0}}}
    };
    
    for (int v = 0; v < 4; v++) {
        bool side1 = false, side2 = false, corner = false;
        
        for (int i = 0; i < 3; i++) {
            int ox = vertexOffsets[face][v][i][0];
            int oy = vertexOffsets[face][v][i][1];
            int oz = vertexOffsets[face][v][i][2];
            
            BlockState neighbor = getNeighborBlock(chunk, x, y, z, ox, oy, oz, world);
            
            if (!neighbor.isAir()) {
                const auto& def = BlockRegistry::get().getBlock(blockDef.getBlockId(neighbor));
                if (def && def->renderType == BlockRenderType::Solid) {
                    if (i == 0) side1 = true;
                    else if (i == 1) side2 = true;
                    else corner = true;
                }
            }
        }
        
        ao[v] = calculateAOTerm(side1, side2, corner);
    }
    
    return ao;
}

uint8_t ChunkMesher::calculateAOTerm(bool side1, bool side2, bool corner) const {
    // AO value: 0 = fully occluded, 3 = no occlusion
    if (side1 && side2) {
        return 0; // Corner fully occluded
    }
    
    // 3 vertices visible = no occlusion (3)
    // 2 vertices visible = slight occlusion (2)
    // 1 vertex visible = medium occlusion (1)
    // 0 vertices visible = full occlusion (0)
    return 3 - (side1 + side2 + corner);
}

std::array<uint32_t, 4> ChunkMesher::calculateVertexLight(
    const Chunk* chunk,
    int x, int y, int z,
    int face,
    World* world) const {
    
    std::array<uint32_t, 4> light;
    
    // Sample light at each vertex position
    // For simplicity, we'll use the center light for now
    // TODO: Implement proper light interpolation
    
    uint8_t skyLight = chunk->getSkyLight(x, y, z);
    uint8_t blockLight = chunk->getBlockLight(x, y, z);
    uint32_t packedLight = packLight(skyLight, blockLight);
    
    light.fill(packedLight);
    
    return light;
}

uint32_t ChunkMesher::packLight(uint8_t skyLight, uint8_t blockLight) const {
    // Pack sky and block light into a single 32-bit value
    // Format: [skyLight:8 | blockLight:8 | padding:16]
    return (static_cast<uint32_t>(skyLight) << 24) |
           (static_cast<uint32_t>(blockLight) << 16);
}

glm::vec4 ChunkMesher::getTextureCoords(BlockState block, int face) const {
    // TODO: Get actual texture coordinates from texture atlas
    // For now, return full texture
    
    if (textureAtlas) {
        // Get texture from atlas
        // return textureAtlas->getCoords(block, face);
    }
    
    // Default: full texture
    return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
}

void ChunkMesher::addQuad(
    ChunkMeshData& mesh,
    const glm::vec3 positions[4],
    const glm::vec3& normal,
    const glm::vec2 uvs[4],
    const std::array<uint8_t, 4>& ao,
    const std::array<uint32_t, 4>& light,
    uint32_t color,
    BlockRenderType renderType) {
    
    // Select the appropriate vertex/index vectors based on render type
    std::vector<ChunkVertex>* vertices;
    std::vector<uint32_t>* indices;
    
    switch (renderType) {
        case BlockRenderType::Cutout:
        case BlockRenderType::CutoutMipped:
            vertices = &mesh.cutoutVertices;
            indices = &mesh.cutoutIndices;
            break;
        case BlockRenderType::Translucent:
            vertices = &mesh.translucentVertices;
            indices = &mesh.translucentIndices;
            break;
        default:
            vertices = &mesh.solidVertices;
            indices = &mesh.solidIndices;
            break;
    }
    
    uint32_t baseIndex = static_cast<uint32_t>(vertices->size());
    
    // Add vertices
    for (int i = 0; i < 4; i++) {
        ChunkVertex vertex;
        vertex.x = positions[i].x;
        vertex.y = positions[i].y;
        vertex.z = positions[i].z;
        vertex.nx = normal.x;
        vertex.ny = normal.y;
        vertex.nz = normal.z;
        vertex.u = uvs[i].x;
        vertex.v = uvs[i].y;
        vertex.ao = static_cast<float>(ao[i]) / 3.0f; // Normalize to 0-1
        vertex.light = light[i];
        vertex.color = color;
        
        vertices->push_back(vertex);
    }
    
    // Add indices (two triangles for the quad)
    // Use AO to determine triangle flip for better shading
    if (ao[0] + ao[2] > ao[1] + ao[3]) {
        indices->push_back(baseIndex + 0);
        indices->push_back(baseIndex + 1);
        indices->push_back(baseIndex + 2);
        indices->push_back(baseIndex + 0);
        indices->push_back(baseIndex + 2);
        indices->push_back(baseIndex + 3);
    } else {
        indices->push_back(baseIndex + 0);
        indices->push_back(baseIndex + 1);
        indices->push_back(baseIndex + 3);
        indices->push_back(baseIndex + 1);
        indices->push_back(baseIndex + 2);
        indices->push_back(baseIndex + 3);
    }
}

// ============================================================================
// ChunkMeshManager Implementation
// ============================================================================

ChunkMeshManager::ChunkMeshManager() {
    LOG_INFO("ChunkMeshManager created");
}

ChunkMeshManager::~ChunkMeshManager() {
    clear();
    LOG_INFO("ChunkMeshManager destroyed");
}

void ChunkMeshManager::init(vk::Device device, vk::PhysicalDevice physicalDevice,
                            vk::Queue queue, vk::CommandPool commandPool) {
    this->device = device;
    this->physicalDevice = physicalDevice;
    this->queue = queue;
    this->commandPool = commandPool;
    
    LOG_INFO("ChunkMeshManager initialized");
}

bool ChunkMeshManager::uploadMesh(const ChunkPos& pos, const ChunkMeshData& meshData) {
    if (meshData.isEmpty()) {
        return true; // Empty mesh is valid
    }
    
    // Remove existing mesh if present
    removeMesh(pos);
    
    ChunkMeshBuffers buffers;
    
    try {
        // Create solid buffers
        if (!meshData.solidVertices.empty()) {
            auto vertexBuffer = VulkanBuffer::createVertexBuffer(
                device, physicalDevice, queue, commandPool,
                meshData.solidVertices.data(),
                meshData.solidVertices.size() * sizeof(ChunkVertex)
            );
            buffers.solidVertexBuffer = vertexBuffer.buffer;
            buffers.solidVertexMemory = vertexBuffer.memory;
            buffers.vertexCount = static_cast<uint32_t>(meshData.solidVertices.size());
            
            auto indexBuffer = VulkanBuffer::createIndexBuffer(
                device, physicalDevice, queue, commandPool,
                meshData.solidIndices.data(),
                meshData.solidIndices.size() * sizeof(uint32_t)
            );
            buffers.solidIndexBuffer = indexBuffer.buffer;
            buffers.solidIndexMemory = indexBuffer.memory;
            buffers.solidIndexCount = static_cast<uint32_t>(meshData.solidIndices.size());
        }
        
        // Create cutout buffers
        if (!meshData.cutoutVertices.empty()) {
            auto vertexBuffer = VulkanBuffer::createVertexBuffer(
                device, physicalDevice, queue, commandPool,
                meshData.cutoutVertices.data(),
                meshData.cutoutVertices.size() * sizeof(ChunkVertex)
            );
            buffers.cutoutVertexBuffer = vertexBuffer.buffer;
            buffers.cutoutVertexMemory = vertexBuffer.memory;
            
            auto indexBuffer = VulkanBuffer::createIndexBuffer(
                device, physicalDevice, queue, commandPool,
                meshData.cutoutIndices.data(),
                meshData.cutoutIndices.size() * sizeof(uint32_t)
            );
            buffers.cutoutIndexBuffer = indexBuffer.buffer;
            buffers.cutoutIndexMemory = indexBuffer.memory;
            buffers.cutoutIndexCount = static_cast<uint32_t>(meshData.cutoutIndices.size());
        }
        
        // Create translucent buffers
        if (!meshData.translucentVertices.empty()) {
            auto vertexBuffer = VulkanBuffer::createVertexBuffer(
                device, physicalDevice, queue, commandPool,
                meshData.translucentVertices.data(),
                meshData.translucentVertices.size() * sizeof(ChunkVertex)
            );
            buffers.translucentVertexBuffer = vertexBuffer.buffer;
            buffers.translucentVertexMemory = vertexBuffer.memory;
            
            auto indexBuffer = VulkanBuffer::createIndexBuffer(
                device, physicalDevice, queue, commandPool,
                meshData.translucentIndices.data(),
                meshData.translucentIndices.size() * sizeof(uint32_t)
            );
            buffers.translucentIndexBuffer = indexBuffer.buffer;
            buffers.translucentIndexMemory = indexBuffer.memory;
            buffers.translucentIndexCount = static_cast<uint32_t>(meshData.translucentIndices.size());
        }
        
        meshes[pos] = std::move(buffers);
        
        // Update memory tracking
        size_t memUsage = 0;
        memUsage += meshData.solidVertices.size() * sizeof(ChunkVertex);
        memUsage += meshData.solidIndices.size() * sizeof(uint32_t);
        memUsage += meshData.cutoutVertices.size() * sizeof(ChunkVertex);
        memUsage += meshData.cutoutIndices.size() * sizeof(uint32_t);
        memUsage += meshData.translucentVertices.size() * sizeof(ChunkVertex);
        memUsage += meshData.translucentIndices.size() * sizeof(uint32_t);
        totalMemoryUsage += memUsage;
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to upload mesh for chunk ({}, {}): {}", 
                  pos.x, pos.z, e.what());
        destroyBuffers(buffers);
        return false;
    }
}

const ChunkMeshBuffers* ChunkMeshManager::getMesh(const ChunkPos& pos) const {
    auto it = meshes.find(pos);
    if (it != meshes.end()) {
        return &it->second;
    }
    return nullptr;
}

void ChunkMeshManager::removeMesh(const ChunkPos& pos) {
    auto it = meshes.find(pos);
    if (it != meshes.end()) {
        destroyBuffers(it->second);
        meshes.erase(it);
    }
}

void ChunkMeshManager::clear() {
    for (auto& pair : meshes) {
        destroyBuffers(pair.second);
    }
    meshes.clear();
    totalMemoryUsage = 0;
}

void ChunkMeshManager::destroyBuffers(ChunkMeshBuffers& buffers) {
    auto destroyBuffer = [this](vk::Buffer buffer, vk::DeviceMemory memory) {
        if (buffer) {
            device.destroyBuffer(buffer);
        }
        if (memory) {
            device.freeMemory(memory);
        }
    };
    
    destroyBuffer(buffers.solidVertexBuffer, buffers.solidVertexMemory);
    destroyBuffer(buffers.solidIndexBuffer, buffers.solidIndexMemory);
    destroyBuffer(buffers.cutoutVertexBuffer, buffers.cutoutVertexMemory);
    destroyBuffer(buffers.cutoutIndexBuffer, buffers.cutoutIndexMemory);
    destroyBuffer(buffers.translucentVertexBuffer, buffers.translucentVertexMemory);
    destroyBuffer(buffers.translucentIndexBuffer, buffers.translucentIndexMemory);
}

} // namespace VoxelForge
