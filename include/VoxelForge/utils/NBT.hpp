/**
 * @file NBT.hpp
 * @brief NBT (Named Binary Tag) serialization system
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <variant>
#include <iostream>

namespace VoxelForge {

// NBT Tag types (matches Minecraft format)
enum class NBTTagType : uint8_t {
    End = 0,
    Byte = 1,
    Short = 2,
    Int = 3,
    Long = 4,
    Float = 5,
    Double = 6,
    ByteArray = 7,
    String = 8,
    List = 9,
    Compound = 10,
    IntArray = 11,
    LongArray = 12
};

// Forward declarations
class NBTTag;
class NBTCompound;
class NBTList;

// Tag value variant
using NBTValue = std::variant<
    std::monostate,           // End
    int8_t,                   // Byte
    int16_t,                  // Short
    int32_t,                  // Int
    int64_t,                  // Long
    float,                    // Float
    double,                   // Double
    std::vector<int8_t>,      // ByteArray
    std::string,              // String
    std::vector<NBTTag>,      // List
    std::unordered_map<std::string, NBTTag>, // Compound
    std::vector<int32_t>,     // IntArray
    std::vector<int64_t>      // LongArray
>;

// NBT Tag
class NBTTag {
public:
    NBTTag() : type(NBTTagType::End), value(std::monostate{}) {}
    
    // Type constructors
    static NBTTag byte(int8_t v);
    static NBTTag shortVal(int16_t v);
    static NBTTag intVal(int32_t v);
    static NBTTag longVal(int64_t v);
    static NBTTag floatVal(float v);
    static NBTTag doubleVal(double v);
    static NBTTag byteArray(const std::vector<int8_t>& v);
    static NBTTag string(const std::string& v);
    static NBTTag list(const std::vector<NBTTag>& v, NBTTagType listType);
    static NBTTag compound();
    static NBTTag intArray(const std::vector<int32_t>& v);
    static NBTTag longArray(const std::vector<int64_t>& v);
    
    NBTTagType getType() const { return type; }
    
    // Value getters
    int8_t asByte() const;
    int16_t asShort() const;
    int32_t asInt() const;
    int64_t asLong() const;
    float asFloat() const;
    double asDouble() const;
    const std::vector<int8_t>& asByteArray() const;
    const std::string& asString() const;
    const std::vector<NBTTag>& asList() const;
    const std::unordered_map<std::string, NBTTag>& asCompound() const;
    const std::vector<int32_t>& asIntArray() const;
    const std::vector<int64_t>& asLongArray() const;
    
    // Compound access
    bool hasKey(const std::string& key) const;
    const NBTTag& get(const std::string& key) const;
    NBTTag& get(const std::string& key);
    
    // Serialization
    std::vector<uint8_t> serialize(const std::string& name = "") const;
    static NBTTag deserialize(const uint8_t* data, size_t size, size_t& offset);
    
    // Debug
    std::string toString(int indent = 0) const;
    
private:
    NBTTagType type;
    NBTValue value;
    NBTTagType listContentType = NBTTagType::End; // For lists
    
    friend class NBTCompound;
    friend class NBTList;
};

// Convenience class for building compounds
class NBTCompound {
public:
    NBTCompound();
    
    NBTCompound& setByte(const std::string& key, int8_t value);
    NBTCompound& setShort(const std::string& key, int16_t value);
    NBTCompound& setInt(const std::string& key, int32_t value);
    NBTCompound& setLong(const std::string& key, int64_t value);
    NBTCompound& setFloat(const std::string& key, float value);
    NBTCompound& setDouble(const std::string& key, double value);
    NBTCompound& setString(const std::string& key, const std::string& value);
    NBTCompound& setByteArray(const std::string& key, const std::vector<int8_t>& value);
    NBTCompound& setIntArray(const std::string& key, const std::vector<int32_t>& value);
    NBTCompound& setLongArray(const std::string& key, const std::vector<int64_t>& value);
    NBTCompound& setCompound(const std::string& key, const NBTCompound& compound);
    NBTCompound& setList(const std::string& key, const NBTList& list);
    
    bool hasKey(const std::string& key) const;
    int8_t getByte(const std::string& key, int8_t defaultVal = 0) const;
    int16_t getShort(const std::string& key, int16_t defaultVal = 0) const;
    int32_t getInt(const std::string& key, int32_t defaultVal = 0) const;
    int64_t getLong(const std::string& key, int64_t defaultVal = 0) const;
    float getFloat(const std::string& key, float defaultVal = 0.0f) const;
    double getDouble(const std::string& key, double defaultVal = 0.0) const;
    std::string getString(const std::string& key, const std::string& defaultVal = "") const;
    
    NBTCompound getCompound(const std::string& key) const;
    
    NBTTag toTag() const;
    static NBTCompound fromTag(const NBTTag& tag);
    
    std::vector<uint8_t> serialize(const std::string& name = "") const;
    static NBTCompound deserialize(const uint8_t* data, size_t size);
    
    void clear();
    bool empty() const;
    size_t size() const;
    
private:
    std::unordered_map<std::string, NBTTag> tags;
};

// Convenience class for building lists
class NBTList {
public:
    NBTList(NBTTagType contentType = NBTTagType::Compound);
    
    NBTList& addByte(int8_t value);
    NBTList& addShort(int16_t value);
    NBTList& addInt(int32_t value);
    NBTList& addLong(int64_t value);
    NBTList& addFloat(float value);
    NBTList& addDouble(double value);
    NBTList& addString(const std::string& value);
    NBTList& addCompound(const NBTCompound& compound);
    
    size_t size() const { return tags.size(); }
    bool empty() const { return tags.empty(); }
    const NBTTag& operator[](size_t index) const;
    
    NBTTag toTag() const;
    
private:
    NBTTagType contentType;
    std::vector<NBTTag> tags;
};

} // namespace VoxelForge
