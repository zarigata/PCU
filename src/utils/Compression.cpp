/**
 * @file Compression.cpp
 * @brief Compression implementation using Zstandard
 */

#include <VoxelForge/utils/Compression.hpp>
#include <VoxelForge/core/Logger.hpp>
#include <zstd.h>
#include <cstring>

namespace VoxelForge {

// ============================================
// Static Compression Functions
// ============================================

std::vector<uint8_t> Compression::compress(const uint8_t* data, size_t size, int level) {
    size_t bound = ZSTD_compressBound(size);
    std::vector<uint8_t> compressed(bound);
    
    size_t result = ZSTD_compress(
        compressed.data(), bound,
        data, size,
        level
    );
    
    if (ZSTD_isError(result)) {
        VF_CORE_ERROR("Compression failed: {}", ZSTD_getErrorName(result));
        return {};
    }
    
    compressed.resize(result);
    return compressed;
}

std::vector<uint8_t> Compression::compress(const std::vector<uint8_t>& data, int level) {
    return compress(data.data(), data.size(), level);
}

bool Compression::decompress(const uint8_t* compressedData, size_t compressedSize,
                            std::vector<uint8_t>& output, size_t expectedSize) {
    output.resize(expectedSize);
    
    size_t result = ZSTD_decompress(
        output.data(), expectedSize,
        compressedData, compressedSize
    );
    
    if (ZSTD_isError(result)) {
        VF_CORE_ERROR("Decompression failed: {}", ZSTD_getErrorName(result));
        return false;
    }
    
    if (result != expectedSize) {
        VF_CORE_WARN("Decompressed size mismatch: expected {}, got {}", expectedSize, result);
    }
    
    output.resize(result);
    return true;
}

std::vector<uint8_t> Compression::decompress(const uint8_t* data, size_t size, size_t expectedSize) {
    std::vector<uint8_t> output;
    if (!decompress(data, size, output, expectedSize)) {
        return {};
    }
    return output;
}

std::vector<uint8_t> Compression::decompress(const std::vector<uint8_t>& compressedData, size_t expectedSize) {
    return decompress(compressedData.data(), compressedData.size(), expectedSize);
}

size_t Compression::getMaxCompressedSize(size_t sourceSize) {
    return ZSTD_compressBound(sourceSize);
}

float Compression::getRatio(size_t originalSize, size_t compressedSize) {
    if (originalSize == 0) return 0.0f;
    return static_cast<float>(compressedSize) / static_cast<float>(originalSize);
}

// ============================================
// Streaming Compressor
// ============================================

struct Compression::Compressor::Impl {
    ZSTD_CStream* stream;
    ZSTD_outBuffer output;
    ZSTD_inBuffer input;
};

Compression::Compressor::Compressor(int level) : level(level), impl(new Impl) {
    impl->stream = ZSTD_createCStream();
    ZSTD_initCStream(impl->stream, level);
}

Compression::Compressor::~Compressor() {
    if (impl->stream) {
        ZSTD_freeCStream(impl->stream);
    }
    delete impl;
}

void Compression::Compressor::begin() {
    buffer.clear();
    ZSTD_initCStream(impl->stream, level);
}

void Compression::Compressor::update(const uint8_t* data, size_t size) {
    impl->input.src = data;
    impl->input.size = size;
    impl->input.pos = 0;
    
    size_t const outSize = ZSTD_CStreamOutSize();
    std::vector<uint8_t> tempBuffer(outSize);
    
    impl->output.dst = tempBuffer.data();
    impl->output.size = outSize;
    
    while (impl->input.pos < impl->input.size) {
        impl->output.pos = 0;
        size_t const ret = ZSTD_compressStream(impl->stream, &impl->output, &impl->input);
        if (ZSTD_isError(ret)) {
            VF_CORE_ERROR("Streaming compression failed: {}", ZSTD_getErrorName(ret));
            return;
        }
        buffer.insert(buffer.end(), tempBuffer.begin(), tempBuffer.begin() + impl->output.pos);
    }
}

std::vector<uint8_t> Compression::Compressor::end() {
    size_t const outSize = ZSTD_CStreamOutSize();
    std::vector<uint8_t> tempBuffer(outSize);
    
    impl->output.dst = tempBuffer.data();
    impl->output.size = outSize;
    impl->output.pos = 0;
    
    size_t const ret = ZSTD_endStream(impl->stream, &impl->output);
    if (ZSTD_isError(ret)) {
        VF_CORE_ERROR("Failed to finalize compression: {}", ZSTD_getErrorName(ret));
        return {};
    }
    
    buffer.insert(buffer.end(), tempBuffer.begin(), tempBuffer.begin() + impl->output.pos);
    return buffer;
}

// ============================================
// Streaming Decompressor
// ============================================

struct Compression::Decompressor::Impl {
    ZSTD_DStream* stream;
};

Compression::Decompressor::Decompressor() : impl(new Impl) {
    impl->stream = ZSTD_createDStream();
    ZSTD_initDStream(impl->stream);
}

Compression::Decompressor::~Decompressor() {
    if (impl->stream) {
        ZSTD_freeDStream(impl->stream);
    }
    delete impl;
}

void Compression::Decompressor::begin() {
    buffer.clear();
    ZSTD_initDStream(impl->stream);
}

void Compression::Decompressor::update(const uint8_t* data, size_t size) {
    ZSTD_inBuffer input = { data, size, 0 };
    
    size_t const outSize = ZSTD_DStreamOutSize();
    std::vector<uint8_t> tempBuffer(outSize);
    
    while (input.pos < input.size) {
        ZSTD_outBuffer output = { tempBuffer.data(), outSize, 0 };
        
        size_t const ret = ZSTD_decompressStream(impl->stream, &output, &input);
        if (ZSTD_isError(ret)) {
            VF_CORE_ERROR("Streaming decompression failed: {}", ZSTD_getErrorName(ret));
            return;
        }
        
        buffer.insert(buffer.end(), tempBuffer.begin(), tempBuffer.begin() + output.pos);
    }
}

std::vector<uint8_t> Compression::Decompressor::end() {
    return buffer;
}

// ============================================
// Fast Compression (LZ4 stub - would need LZ4 library)
// ============================================

std::vector<uint8_t> FastCompression::compress(const uint8_t* data, size_t size) {
    // Fallback to Zstd at level 1 for speed
    return Compression::compress(data, size, Compression::FAST);
}

std::vector<uint8_t> FastCompression::compress(const std::vector<uint8_t>& data) {
    return compress(data.data(), data.size());
}

std::vector<uint8_t> FastCompression::decompress(const uint8_t* data, size_t size, size_t expectedSize) {
    return Compression::decompress(data, size, expectedSize);
}

std::vector<uint8_t> FastCompression::decompress(const std::vector<uint8_t>& compressedData, size_t expectedSize) {
    return decompress(compressedData.data(), compressedData.size(), expectedSize);
}

size_t FastCompression::getMaxCompressedSize(size_t sourceSize) {
    return Compression::getMaxCompressedSize(sourceSize);
}

} // namespace VoxelForge
