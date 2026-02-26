/**
 * @file Compression.hpp
 * @brief Compression utilities using Zstandard
 */

#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace VoxelForge {

class Compression {
public:
    // Compression level: 1 (fast) to 22 (max compression)
    static constexpr int FAST = 1;
    static constexpr int DEFAULT = 3;
    static constexpr int BALANCED = 10;
    static constexpr int MAX = 22;
    
    // Compress data
    static std::vector<uint8_t> compress(const uint8_t* data, size_t size, int level = DEFAULT);
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& data, int level = DEFAULT);
    
    // Decompress data
    static bool decompress(const uint8_t* compressedData, size_t compressedSize,
                          std::vector<uint8_t>& output, size_t expectedSize);
    static std::vector<uint8_t> decompress(const uint8_t* data, size_t size, size_t expectedSize);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressedData, size_t expectedSize);
    
    // Get maximum compressed size
    static size_t getMaxCompressedSize(size_t sourceSize);
    
    // Get compression ratio
    static float getRatio(size_t originalSize, size_t compressedSize);
    
    // Streaming compression for large data
    class Compressor {
    public:
        explicit Compressor(int level = DEFAULT);
        ~Compressor();
        
        void begin();
        void update(const uint8_t* data, size_t size);
        std::vector<uint8_t> end();
        
    private:
        struct Impl;
        Impl* impl;
        int level;
        std::vector<uint8_t> buffer;
    };
    
    // Streaming decompression
    class Decompressor {
    public:
        Decompressor();
        ~Decompressor();
        
        void begin();
        void update(const uint8_t* data, size_t size);
        std::vector<uint8_t> end();
        
    private:
        struct Impl;
        Impl* impl;
        std::vector<uint8_t> buffer;
    };
};

// Fast LZ4 compression for real-time use
class FastCompression {
public:
    static std::vector<uint8_t> compress(const uint8_t* data, size_t size);
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& data);
    
    static std::vector<uint8_t> decompress(const uint8_t* data, size_t size, size_t expectedSize);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressedData, size_t expectedSize);
    
    static size_t getMaxCompressedSize(size_t sourceSize);
};

} // namespace VoxelForge
