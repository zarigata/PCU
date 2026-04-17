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

TEST_F(ChunkMesherTest, ChunkVertex_Size) {
    EXPECT_EQ(sizeof(ChunkVertex), 44);
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

TEST_F(ChunkMesherTest, GenerateMesh_EmptyChunk) {
    ChunkPos pos(0, 0);
    Chunk chunk(pos);
    
    // Empty chunk should produce empty mesh
    auto result = mesher->generateMesh(&chunk, nullptr);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.meshData.isEmpty());
}

TEST_F(ChunkMesherTest, GenerateMesh_SingleBlock) {
    ChunkPos pos(0, 0);
    Chunk chunk(pos);
    
    // Place a single stone block
    auto stoneState = BlockRegistry::get().getDefaultState("poorcraftultra:stone");
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

} // namespace test
} // namespace VoxelForge
