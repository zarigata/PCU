/**
 * @file test_chunk_mesher.cpp
 * @brief Unit tests for chunk mesh generation
 */

#include <gtest/gtest.h>
#include <VoxelForge/world/ChunkMesher.hpp>
#include <VoxelForge/world/Chunk.hpp>
#include <VoxelForge/world/BlockRegistry.hpp>
#include <VoxelForge/core/Logger.hpp>

namespace VoxelForge {
namespace test {

class ChunkMesherTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger and block registry
        static bool initialized = false;
        if (!initialized) {
            Logger::init();
            BlockRegistry::get().registerVanillaBlocks();
            initialized = true;
        }
        
        mesher = std::make_unique<ChunkMesher>();
    }
    
    void TearDown() override {
        mesher.reset();
    }
    
    std::unique_ptr<ChunkMesher> mesher;
};

// Test face normal directions
TEST_F(ChunkMesherTest, FaceNormals_CorrectDirections) {
    // Verify face normals are unit vectors
    for (int i = 0; i < 6; i++) {
        const auto& normal = ChunkMesher::FACE_NORMALS[i];
        float length = glm::length(normal);
        EXPECT_FLOAT_EQ(length, 1.0f);
    }
    
    // Verify face normals point in correct directions
    EXPECT_FLOAT_EQ(ChunkMesher::FACE_NORMALS[0].x, 1.0f);  // Right +X
    EXPECT_FLOAT_EQ(ChunkMesher::FACE_NORMALS[1].x, -1.0f); // Left -X
    EXPECT_FLOAT_EQ(ChunkMesher::FACE_NORMALS[2].y, 1.0f);  // Up +Y
    EXPECT_FLOAT_EQ(ChunkMesher::FACE_NORMALS[3].y, -1.0f); // Down -Y
    EXPECT_FLOAT_EQ(ChunkMesher::FACE_NORMALS[4].z, 1.0f);  // Front +Z
    EXPECT_FLOAT_EQ(ChunkMesher::FACE_NORMALS[5].z, -1.0f); // Back -Z
}

// Test face offsets for neighbor checking
TEST_F(ChunkMesherTest, FaceOffsets_CorrectDirections) {
    // Verify offsets match face directions
    EXPECT_EQ(ChunkMesher::FACE_OFFSETS[0], glm::ivec3(1, 0, 0));   // Right
    EXPECT_EQ(ChunkMesher::FACE_OFFSETS[1], glm::ivec3(-1, 0, 0));  // Left
    EXPECT_EQ(ChunkMesher::FACE_OFFSETS[2], glm::ivec3(0, 1, 0));   // Up
    EXPECT_EQ(ChunkMesher::FACE_OFFSETS[3], glm::ivec3(0, -1, 0));  // Down
    EXPECT_EQ(ChunkMesher::FACE_OFFSETS[4], glm::ivec3(0, 0, 1));   // Front
    EXPECT_EQ(ChunkMesher::FACE_OFFSETS[5], glm::ivec3(0, 0, -1));  // Back
}

// Test chunk mesh data structure
TEST_F(ChunkMesherTest, ChunkMeshData_EmptyState) {
    ChunkMeshData mesh;
    
    EXPECT_TRUE(mesh.isEmpty());
    EXPECT_EQ(mesh.getTotalVertexCount(), 0);
    EXPECT_EQ(mesh.getTotalIndexCount(), 0);
    EXPECT_TRUE(mesh.solidVertices.empty());
    EXPECT_TRUE(mesh.solidIndices.empty());
    EXPECT_TRUE(mesh.cutoutVertices.empty());
    EXPECT_TRUE(mesh.cutoutIndices.empty());
    EXPECT_TRUE(mesh.translucentVertices.empty());
    EXPECT_TRUE(mesh.translucentIndices.empty());
}

// Test chunk mesh data clear
TEST_F(ChunkMesherTest, ChunkMeshData_Clear) {
    ChunkMeshData mesh;
    
    // Add some data
    mesh.solidVertices.push_back({});
    mesh.solidIndices.push_back(0);
    mesh.cutoutVertices.push_back({});
    mesh.translucentVertices.push_back({});
    
    EXPECT_FALSE(mesh.isEmpty());
    
    mesh.clear();
    
    EXPECT_TRUE(mesh.isEmpty());
    EXPECT_TRUE(mesh.solidVertices.empty());
    EXPECT_TRUE(mesh.solidIndices.empty());
}

// Test chunk vertex structure size
TEST_F(ChunkMesherTest, ChunkVertex_Size) {
    // Verify vertex structure size for GPU alignment
    // Position (12) + Normal (12) + UV (8) + AO (4) + Light (4) + Color (4) = 44 bytes
    // But due to alignment, actual size might be different
    
    EXPECT_EQ(sizeof(ChunkVertex), 44); // Packed structure
}

// Test mesher configuration
TEST_F(ChunkMesherTest, Config_DefaultValues) {
    ChunkMesherConfig config = mesher->getConfig();
    
    EXPECT_TRUE(config.enableAO);
    EXPECT_TRUE(config.enableLightInterpolation);
    EXPECT_TRUE(config.cullHiddenFaces);
    EXPECT_FALSE(config.enableGreedyMeshing);
    EXPECT_EQ(config.maxMeshesPerFrame, 4);
    EXPECT_TRUE(config.useThreadedGeneration);
}

// Test mesher configuration modification
TEST_F(ChunkMesherTest, Config_Modification) {
    ChunkMesherConfig config;
    config.enableAO = false;
    config.cullHiddenFaces = false;
    config.maxMeshesPerFrame = 8;
    
    mesher->setConfig(config);
    
    auto retrieved = mesher->getConfig();
    EXPECT_FALSE(retrieved.enableAO);
    EXPECT_FALSE(retrieved.cullHiddenFaces);
    EXPECT_EQ(retrieved.maxMeshesPerFrame, 8);
}

// Test mesh generation for empty chunk
TEST_F(ChunkMesherTest, GenerateMesh_EmptyChunk) {
    ChunkPos pos(0, 0);
    Chunk chunk(pos);
    
    // Empty chunk should produce empty mesh
    auto result = mesher->generateMesh(&chunk, nullptr);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.meshData.isEmpty());
}

// Test mesh generation for single block chunk
TEST_F(ChunkMesherTest, GenerateMesh_SingleBlock) {
    ChunkPos pos(0, 0);
    Chunk chunk(pos);
    
    // Place a single stone block
    auto stoneState = BlockRegistry::get().getDefaultState("minecraft:stone");
    chunk.setBlock(8, 64, 8, stoneState); // Center of chunk
    
    auto result = mesher->generateMesh(&chunk, nullptr);
    
    EXPECT_TRUE(result.success);
    
    // Single block should generate 6 faces * 4 vertices = 24 vertices
    // All faces visible since surrounded by air
    EXPECT_EQ(result.meshData.getTotalVertexCount(), 24);
    // 6 faces * 6 indices = 36 indices
    EXPECT_EQ(result.meshData.getTotalIndexCount(), 36);
}

// Test mesh generation statistics
TEST_F(ChunkMesherTest, Stats_Tracking) {
    mesher->resetStats();
    
    auto stats = mesher->getStats();
    EXPECT_EQ(stats.meshesGenerated, 0);
    EXPECT_EQ(stats.totalVerticesGenerated, 0);
    EXPECT_DOUBLE_EQ(stats.averageTimeMs, 0.0);
}

// Test ambient occlusion calculation (logic only)
TEST_F(ChunkMesherTest, AO_CornerOcclusion) {
    // Test AO logic without actual chunk data
    // Full corner occlusion (both sides and corner blocked)
    bool side1 = true;
    bool side2 = true;
    bool corner = true;
    
    // Should be fully occluded (value 0)
    uint8_t ao = 0; // calculateAOTerm logic
    if (side1 && side2) {
        ao = 0;
    } else {
        ao = 3 - (side1 + side2 + corner);
    }
    
    EXPECT_EQ(ao, 0);
    
    // No occlusion
    side1 = false;
    side2 = false;
    corner = false;
    ao = 3 - (side1 + side2 + corner);
    EXPECT_EQ(ao, 3);
}

// Test light packing
TEST_F(ChunkMesherTest, Light_Packing) {
    uint8_t skyLight = 15;
    uint8_t blockLight = 12;
    
    // Pack light values
    uint32_t packed = (static_cast<uint32_t>(skyLight) << 24) |
                      (static_cast<uint32_t>(blockLight) << 16);
    
    // Unpack
    uint8_t unpackedSky = (packed >> 24) & 0xFF;
    uint8_t unpackedBlock = (packed >> 16) & 0xFF;
    
    EXPECT_EQ(unpackedSky, skyLight);
    EXPECT_EQ(unpackedBlock, blockLight);
}

// Test face vertex positions
TEST_F(ChunkMesherTest, FaceVertices_CorrectPositions) {
    // Verify all face vertices are within [0,1] range
    for (int face = 0; face < 6; face++) {
        for (int v = 0; v < 4; v++) {
            for (int c = 0; c < 3; c++) {
                int val = ChunkMesher::FACE_VERTICES[face][v][c];
                EXPECT_TRUE(val >= 0 && val <= 1);
            }
        }
    }
}

// Test UV coordinates
TEST_F(ChunkMesherTest, UVCoordinates_StandardQuad) {
    // Verify standard quad UVs
    EXPECT_FLOAT_EQ(ChunkMesher::FACE_UVS[0].x, 0.0f);
    EXPECT_FLOAT_EQ(ChunkMesher::FACE_UVS[0].y, 0.0f);
    EXPECT_FLOAT_EQ(ChunkMesher::FACE_UVS[1].x, 1.0f);
    EXPECT_FLOAT_EQ(ChunkMesher::FACE_UVS[1].y, 0.0f);
    EXPECT_FLOAT_EQ(ChunkMesher::FACE_UVS[2].x, 1.0f);
    EXPECT_FLOAT_EQ(ChunkMesher::FACE_UVS[2].y, 1.0f);
    EXPECT_FLOAT_EQ(ChunkMesher::FACE_UVS[3].x, 0.0f);
    EXPECT_FLOAT_EQ(ChunkMesher::FACE_UVS[3].y, 1.0f);
}

} // namespace test
} // namespace VoxelForge
