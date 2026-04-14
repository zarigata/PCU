/**
 * @file NBT.cpp
 * @brief NBT implementation
 */

#include <VoxelForge/utils/NBT.hpp>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace VoxelForge {

// ============================================
// NBTTag
// ============================================

NBTTag NBTTag::byte(int8_t v) {
    NBTTag tag;
    tag.type = NBTTagType::Byte;
    tag.value = v;
    return tag;
}

NBTTag NBTTag::shortVal(int16_t v) {
    NBTTag tag;
    tag.type = NBTTagType::Short;
    tag.value = v;
    return tag;
}

NBTTag NBTTag::intVal(int32_t v) {
    NBTTag tag;
    tag.type = NBTTagType::Int;
    tag.value = v;
    return tag;
}

NBTTag NBTTag::longVal(int64_t v) {
    NBTTag tag;
    tag.type = NBTTagType::Long;
    tag.value = v;
    return tag;
}

NBTTag NBTTag::floatVal(float v) {
    NBTTag tag;
    tag.type = NBTTagType::Float;
    tag.value = v;
    return tag;
}

NBTTag NBTTag::doubleVal(double v) {
    NBTTag tag;
    tag.type = NBTTagType::Double;
    tag.value = v;
    return tag;
}

NBTTag NBTTag::byteArray(const std::vector<int8_t>& v) {
    NBTTag tag;
    tag.type = NBTTagType::ByteArray;
    tag.value = v;
    return tag;
}

NBTTag NBTTag::string(const std::string& v) {
    NBTTag tag;
    tag.type = NBTTagType::String;
    tag.value = v;
    return tag;
}

NBTTag NBTTag::list(const std::vector<NBTTag>& v, NBTTagType listType) {
    NBTTag tag;
    tag.type = NBTTagType::List;
    tag.value = v;
    tag.listContentType = listType;
    return tag;
}

NBTTag NBTTag::compound() {
    NBTTag tag;
    tag.type = NBTTagType::Compound;
    tag.value = std::unordered_map<std::string, NBTTag>();
    return tag;
}

NBTTag NBTTag::intArray(const std::vector<int32_t>& v) {
    NBTTag tag;
    tag.type = NBTTagType::IntArray;
    tag.value = v;
    return tag;
}

NBTTag NBTTag::longArray(const std::vector<int64_t>& v) {
    NBTTag tag;
    tag.type = NBTTagType::LongArray;
    tag.value = v;
    return tag;
}

int8_t NBTTag::asByte() const {
    return std::get<int8_t>(value);
}

int16_t NBTTag::asShort() const {
    return std::get<int16_t>(value);
}

int32_t NBTTag::asInt() const {
    return std::get<int32_t>(value);
}

int64_t NBTTag::asLong() const {
    return std::get<int64_t>(value);
}

float NBTTag::asFloat() const {
    return std::get<float>(value);
}

double NBTTag::asDouble() const {
    return std::get<double>(value);
}

const std::vector<int8_t>& NBTTag::asByteArray() const {
    return std::get<std::vector<int8_t>>(value);
}

const std::string& NBTTag::asString() const {
    return std::get<std::string>(value);
}

const std::vector<NBTTag>& NBTTag::asList() const {
    return std::get<std::vector<NBTTag>>(value);
}

const std::unordered_map<std::string, NBTTag>& NBTTag::asCompound() const {
    return std::get<std::unordered_map<std::string, NBTTag>>(value);
}

const std::vector<int32_t>& NBTTag::asIntArray() const {
    return std::get<std::vector<int32_t>>(value);
}

const std::vector<int64_t>& NBTTag::asLongArray() const {
    return std::get<std::vector<int64_t>>(value);
}

bool NBTTag::hasKey(const std::string& key) const {
    if (type != NBTTagType::Compound) return false;
    auto& compound = std::get<std::unordered_map<std::string, NBTTag>>(value);
    return compound.find(key) != compound.end();
}

const NBTTag& NBTTag::get(const std::string& key) const {
    static NBTTag empty;
    if (type != NBTTagType::Compound) return empty;
    auto& compound = std::get<std::unordered_map<std::string, NBTTag>>(value);
    auto it = compound.find(key);
    return it != compound.end() ? it->second : empty;
}

NBTTag& NBTTag::get(const std::string& key) {
    static NBTTag empty;
    if (type != NBTTagType::Compound) return empty;
    auto& compound = std::get<std::unordered_map<std::string, NBTTag>>(value);
    return compound[key];
}

std::string NBTTag::toString(int indent) const {
    std::string pad(indent, ' ');
    std::stringstream ss;
    
    switch (type) {
        case NBTTagType::End:
            ss << pad << "TAG_End";
            break;
        case NBTTagType::Byte:
            ss << pad << "TAG_Byte: " << static_cast<int>(asByte());
            break;
        case NBTTagType::Short:
            ss << pad << "TAG_Short: " << asShort();
            break;
        case NBTTagType::Int:
            ss << pad << "TAG_Int: " << asInt();
            break;
        case NBTTagType::Long:
            ss << pad << "TAG_Long: " << asLong();
            break;
        case NBTTagType::Float:
            ss << pad << "TAG_Float: " << asFloat();
            break;
        case NBTTagType::Double:
            ss << pad << "TAG_Double: " << asDouble();
            break;
        case NBTTagType::String:
            ss << pad << "TAG_String: \"" << asString() << "\"";
            break;
        case NBTTagType::Compound:
            ss << pad << "TAG_Compound (" << asCompound().size() << " entries) {";
            for (const auto& [key, tag] : asCompound()) {
                ss << "\n" << pad << "  " << key << ": " << tag.toString(indent + 4);
            }
            ss << "\n" << pad << "}";
            break;
        case NBTTagType::List:
            ss << pad << "TAG_List (" << asList().size() << " entries) [";
            for (const auto& tag : asList()) {
                ss << "\n" << tag.toString(indent + 2);
            }
            ss << "\n" << pad << "]";
            break;
        default:
            ss << pad << "TAG_Unknown";
    }
    
    return ss.str();
}

// ============================================
// NBTCompound
// ============================================

NBTCompound::NBTCompound() {}

NBTCompound& NBTCompound::setByte(const std::string& key, int8_t value) {
    tags[key] = NBTTag::byte(value);
    return *this;
}

NBTCompound& NBTCompound::setShort(const std::string& key, int16_t value) {
    tags[key] = NBTTag::shortVal(value);
    return *this;
}

NBTCompound& NBTCompound::setInt(const std::string& key, int32_t value) {
    tags[key] = NBTTag::intVal(value);
    return *this;
}

NBTCompound& NBTCompound::setLong(const std::string& key, int64_t value) {
    tags[key] = NBTTag::longVal(value);
    return *this;
}

NBTCompound& NBTCompound::setFloat(const std::string& key, float value) {
    tags[key] = NBTTag::floatVal(value);
    return *this;
}

NBTCompound& NBTCompound::setDouble(const std::string& key, double value) {
    tags[key] = NBTTag::doubleVal(value);
    return *this;
}

NBTCompound& NBTCompound::setString(const std::string& key, const std::string& value) {
    tags[key] = NBTTag::string(value);
    return *this;
}

NBTCompound& NBTCompound::setByteArray(const std::string& key, const std::vector<int8_t>& value) {
    tags[key] = NBTTag::byteArray(value);
    return *this;
}

NBTCompound& NBTCompound::setIntArray(const std::string& key, const std::vector<int32_t>& value) {
    tags[key] = NBTTag::intArray(value);
    return *this;
}

NBTCompound& NBTCompound::setLongArray(const std::string& key, const std::vector<int64_t>& value) {
    tags[key] = NBTTag::longArray(value);
    return *this;
}

NBTCompound& NBTCompound::setCompound(const std::string& key, const NBTCompound& compound) {
    tags[key] = compound.toTag();
    return *this;
}

NBTCompound& NBTCompound::setList(const std::string& key, const NBTList& list) {
    tags[key] = list.toTag();
    return *this;
}

bool NBTCompound::hasKey(const std::string& key) const {
    return tags.find(key) != tags.end();
}

int8_t NBTCompound::getByte(const std::string& key, int8_t defaultVal) const {
    auto it = tags.find(key);
    if (it != tags.end() && it->second.getType() == NBTTagType::Byte) {
        return it->second.asByte();
    }
    return defaultVal;
}

int16_t NBTCompound::getShort(const std::string& key, int16_t defaultVal) const {
    auto it = tags.find(key);
    if (it != tags.end() && it->second.getType() == NBTTagType::Short) {
        return it->second.asShort();
    }
    return defaultVal;
}

int32_t NBTCompound::getInt(const std::string& key, int32_t defaultVal) const {
    auto it = tags.find(key);
    if (it != tags.end() && it->second.getType() == NBTTagType::Int) {
        return it->second.asInt();
    }
    return defaultVal;
}

int64_t NBTCompound::getLong(const std::string& key, int64_t defaultVal) const {
    auto it = tags.find(key);
    if (it != tags.end() && it->second.getType() == NBTTagType::Long) {
        return it->second.asLong();
    }
    return defaultVal;
}

float NBTCompound::getFloat(const std::string& key, float defaultVal) const {
    auto it = tags.find(key);
    if (it != tags.end() && it->second.getType() == NBTTagType::Float) {
        return it->second.asFloat();
    }
    return defaultVal;
}

double NBTCompound::getDouble(const std::string& key, double defaultVal) const {
    auto it = tags.find(key);
    if (it != tags.end() && it->second.getType() == NBTTagType::Double) {
        return it->second.asDouble();
    }
    return defaultVal;
}

std::string NBTCompound::getString(const std::string& key, const std::string& defaultVal) const {
    auto it = tags.find(key);
    if (it != tags.end() && it->second.getType() == NBTTagType::String) {
        return it->second.asString();
    }
    return defaultVal;
}

NBTCompound NBTCompound::getCompound(const std::string& key) const {
    auto it = tags.find(key);
    if (it != tags.end() && it->second.getType() == NBTTagType::Compound) {
        return fromTag(it->second);
    }
    return NBTCompound();
}

NBTTag NBTCompound::toTag() const {
    auto tag = NBTTag::compound();
    tag.value = tags;
    return tag;
}

NBTCompound NBTCompound::fromTag(const NBTTag& tag) {
    NBTCompound compound;
    if (tag.getType() == NBTTagType::Compound) {
        compound.tags = tag.asCompound();
    }
    return compound;
}

void NBTCompound::clear() {
    tags.clear();
}

bool NBTCompound::empty() const {
    return tags.empty();
}

size_t NBTCompound::size() const {
    return tags.size();
}

// ============================================
// NBTList
// ============================================

NBTList::NBTList(NBTTagType contentType) : contentType(contentType) {}

NBTList& NBTList::addByte(int8_t value) {
    tags.push_back(NBTTag::byte(value));
    return *this;
}

NBTList& NBTList::addShort(int16_t value) {
    tags.push_back(NBTTag::shortVal(value));
    return *this;
}

NBTList& NBTList::addInt(int32_t value) {
    tags.push_back(NBTTag::intVal(value));
    return *this;
}

NBTList& NBTList::addLong(int64_t value) {
    tags.push_back(NBTTag::longVal(value));
    return *this;
}

NBTList& NBTList::addFloat(float value) {
    tags.push_back(NBTTag::floatVal(value));
    return *this;
}

NBTList& NBTList::addDouble(double value) {
    tags.push_back(NBTTag::doubleVal(value));
    return *this;
}

NBTList& NBTList::addString(const std::string& value) {
    tags.push_back(NBTTag::string(value));
    return *this;
}

NBTList& NBTList::addCompound(const NBTCompound& compound) {
    tags.push_back(compound.toTag());
    return *this;
}

const NBTTag& NBTList::operator[](size_t index) const {
    static NBTTag empty;
    if (index >= tags.size()) return empty;
    return tags[index];
}

NBTTag NBTList::toTag() const {
    return NBTTag::list(tags, contentType);
}

// ============================================
// NBT Serialization Helpers
// ============================================

static void writeBigEndian16(std::vector<uint8_t>& out, uint16_t val) {
    out.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(val & 0xFF));
}

static void writeBigEndian32(std::vector<uint8_t>& out, uint32_t val) {
    out.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
    out.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(val & 0xFF));
}

static void writeBigEndian64(std::vector<uint8_t>& out, uint64_t val) {
    for (int i = 56; i >= 0; i -= 8)
        out.push_back(static_cast<uint8_t>((val >> i) & 0xFF));
}

static void writeModifiedUTF8(std::vector<uint8_t>& out, const std::string& s) {
    writeBigEndian16(out, static_cast<uint16_t>(s.size()));
    out.insert(out.end(), s.begin(), s.end());
}

static void writePayload(std::vector<uint8_t>& out, const NBTTag& tag);

static void writeCompoundPayload(std::vector<uint8_t>& out,
                                 const std::unordered_map<std::string, NBTTag>& compound) {
    for (const auto& [key, child] : compound) {
        out.push_back(static_cast<uint8_t>(child.getType()));
        writeModifiedUTF8(out, key);
        writePayload(out, child);
    }
    // TAG_End
    out.push_back(0);
}

static void writePayload(std::vector<uint8_t>& out, const NBTTag& tag) {
    switch (tag.getType()) {
        case NBTTagType::Byte:
            out.push_back(static_cast<uint8_t>(tag.asByte()));
            break;
        case NBTTagType::Short: {
            int16_t v = tag.asShort();
            writeBigEndian16(out, static_cast<uint16_t>(v));
            break;
        }
        case NBTTagType::Int:
            writeBigEndian32(out, static_cast<uint32_t>(tag.asInt()));
            break;
        case NBTTagType::Long:
            writeBigEndian64(out, static_cast<uint64_t>(tag.asLong()));
            break;
        case NBTTagType::Float: {
            float v = tag.asFloat();
            uint32_t bits;
            std::memcpy(&bits, &v, 4);
            writeBigEndian32(out, bits);
            break;
        }
        case NBTTagType::Double: {
            double v = tag.asDouble();
            uint64_t bits;
            std::memcpy(&bits, &v, 8);
            writeBigEndian64(out, bits);
            break;
        }
        case NBTTagType::ByteArray: {
            const auto& arr = tag.asByteArray();
            writeBigEndian32(out, static_cast<uint32_t>(arr.size()));
            out.insert(out.end(), reinterpret_cast<const uint8_t*>(arr.data()),
                       reinterpret_cast<const uint8_t*>(arr.data()) + arr.size());
            break;
        }
        case NBTTagType::String:
            writeModifiedUTF8(out, tag.asString());
            break;
        case NBTTagType::List: {
            const auto& list = tag.asList();
            // Determine content type from first element or End if empty
            NBTTagType contentType = NBTTagType::End;
            if (!list.empty()) contentType = list[0].getType();
            out.push_back(static_cast<uint8_t>(contentType));
            writeBigEndian32(out, static_cast<uint32_t>(list.size()));
            for (const auto& item : list)
                writePayload(out, item);
            break;
        }
        case NBTTagType::Compound:
            writeCompoundPayload(out, tag.asCompound());
            break;
        case NBTTagType::IntArray: {
            const auto& arr = tag.asIntArray();
            writeBigEndian32(out, static_cast<uint32_t>(arr.size()));
            for (int32_t v : arr)
                writeBigEndian32(out, static_cast<uint32_t>(v));
            break;
        }
        case NBTTagType::LongArray: {
            const auto& arr = tag.asLongArray();
            writeBigEndian32(out, static_cast<uint32_t>(arr.size()));
            for (int64_t v : arr)
                writeBigEndian64(out, static_cast<uint64_t>(v));
            break;
        }
        case NBTTagType::End:
        default:
            break;
    }
}

// ============================================
// NBTTag Serialization
// ============================================

std::vector<uint8_t> NBTTag::serialize(const std::string& name) const {
    std::vector<uint8_t> out;
    // Tag type byte
    out.push_back(static_cast<uint8_t>(type));
    // Name (modified UTF-8)
    writeModifiedUTF8(out, name);
    // Payload
    writePayload(out, *this);
    return out;
}

// ============================================
// NBTCompound Serialization
// ============================================

std::vector<uint8_t> NBTCompound::serialize(const std::string& name) const {
    return toTag().serialize(name);
}

// ============================================
// NBT Deserialization Helpers
// ============================================

static uint8_t readU8(const uint8_t* data, size_t size, size_t& offset, bool& ok) {
    if (offset >= size) { ok = false; return 0; }
    return data[offset++];
}

static uint16_t readBE16(const uint8_t* data, size_t size, size_t& offset, bool& ok) {
    if (offset + 1 >= size) { ok = false; return 0; }
    uint16_t v = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
    offset += 2;
    return v;
}

static uint32_t readBE32(const uint8_t* data, size_t size, size_t& offset, bool& ok) {
    if (offset + 3 >= size) { ok = false; return 0; }
    uint32_t v = (static_cast<uint32_t>(data[offset]) << 24) |
                 (static_cast<uint32_t>(data[offset + 1]) << 16) |
                 (static_cast<uint32_t>(data[offset + 2]) << 8) |
                 static_cast<uint32_t>(data[offset + 3]);
    offset += 4;
    return v;
}

static uint64_t readBE64(const uint8_t* data, size_t size, size_t& offset, bool& ok) {
    if (offset + 7 >= size) { ok = false; return 0; }
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i)
        v = (v << 8) | data[offset++];
    return v;
}

static std::string readModifiedUTF8(const uint8_t* data, size_t size, size_t& offset, bool& ok) {
    uint16_t len = readBE16(data, size, offset, ok);
    if (!ok || offset + len > size) { ok = false; return ""; }
    std::string s(reinterpret_cast<const char*>(data + offset), len);
    offset += len;
    return s;
}

static NBTTag readPayload(NBTTagType type, const uint8_t* data, size_t size, size_t& offset, bool& ok);

static std::unordered_map<std::string, NBTTag> readCompoundPayload(
        const uint8_t* data, size_t size, size_t& offset, bool& ok) {
    std::unordered_map<std::string, NBTTag> compound;
    while (ok) {
        uint8_t typeByte = readU8(data, size, offset, ok);
        if (!ok || typeByte == 0) break; // TAG_End
        auto childType = static_cast<NBTTagType>(typeByte);
        std::string name = readModifiedUTF8(data, size, offset, ok);
        if (!ok) break;
        NBTTag child = readPayload(childType, data, size, offset, ok);
        if (!ok) break;
        compound[name] = std::move(child);
    }
    return compound;
}

static NBTTag readPayload(NBTTagType type, const uint8_t* data, size_t size, size_t& offset, bool& ok) {
    switch (type) {
        case NBTTagType::Byte: {
            int8_t v = static_cast<int8_t>(readU8(data, size, offset, ok));
            return NBTTag::byte(v);
        }
        case NBTTagType::Short: {
            int16_t v = static_cast<int16_t>(readBE16(data, size, offset, ok));
            return NBTTag::shortVal(v);
        }
        case NBTTagType::Int: {
            int32_t v = static_cast<int32_t>(readBE32(data, size, offset, ok));
            return NBTTag::intVal(v);
        }
        case NBTTagType::Long: {
            int64_t v = static_cast<int64_t>(readBE64(data, size, offset, ok));
            return NBTTag::longVal(v);
        }
        case NBTTagType::Float: {
            uint32_t bits = readBE32(data, size, offset, ok);
            float v;
            std::memcpy(&v, &bits, 4);
            return NBTTag::floatVal(v);
        }
        case NBTTagType::Double: {
            uint64_t bits = readBE64(data, size, offset, ok);
            double v;
            std::memcpy(&v, &bits, 8);
            return NBTTag::doubleVal(v);
        }
        case NBTTagType::ByteArray: {
            uint32_t len = readBE32(data, size, offset, ok);
            if (!ok || offset + len > size) { ok = false; return NBTTag(); }
            std::vector<int8_t> arr(reinterpret_cast<const int8_t*>(data + offset),
                                     reinterpret_cast<const int8_t*>(data + offset + len));
            offset += len;
            return NBTTag::byteArray(arr);
        }
        case NBTTagType::String: {
            std::string s = readModifiedUTF8(data, size, offset, ok);
            return NBTTag::string(s);
        }
        case NBTTagType::List: {
            uint8_t contentTypeByte = readU8(data, size, offset, ok);
            auto contentType = static_cast<NBTTagType>(contentTypeByte);
            uint32_t listLen = readBE32(data, size, offset, ok);
            std::vector<NBTTag> items;
            items.reserve(listLen);
            for (uint32_t i = 0; i < listLen && ok; ++i)
                items.push_back(readPayload(contentType, data, size, offset, ok));
            return NBTTag::list(items, contentType);
        }
        case NBTTagType::Compound: {
            auto compoundData = readCompoundPayload(data, size, offset, ok);
            return NBTTag::fromCompound(std::move(compoundData));
        }
        case NBTTagType::IntArray: {
            uint32_t len = readBE32(data, size, offset, ok);
            std::vector<int32_t> arr;
            arr.reserve(len);
            for (uint32_t i = 0; i < len && ok; ++i)
                arr.push_back(static_cast<int32_t>(readBE32(data, size, offset, ok)));
            return NBTTag::intArray(arr);
        }
        case NBTTagType::LongArray: {
            uint32_t len = readBE32(data, size, offset, ok);
            std::vector<int64_t> arr;
            arr.reserve(len);
            for (uint32_t i = 0; i < len && ok; ++i)
                arr.push_back(static_cast<int64_t>(readBE64(data, size, offset, ok)));
            return NBTTag::longArray(arr);
        }
        default:
            ok = false;
            return NBTTag();
    }
}

NBTTag NBTTag::deserialize(const uint8_t* data, size_t size, size_t& offset) {
    bool ok = true;
    uint8_t typeByte = readU8(data, size, offset, ok);
    if (!ok) return NBTTag();
    auto tagType = static_cast<NBTTagType>(typeByte);
    // Read name
    [[maybe_unused]] std::string name = readModifiedUTF8(data, size, offset, ok);
    if (!ok) return NBTTag();
    return readPayload(tagType, data, size, offset, ok);
}

NBTCompound NBTCompound::deserialize(const uint8_t* data, size_t size) {
    size_t offset = 0;
    bool ok = true;
    uint8_t typeByte = readU8(data, size, offset, ok);
    if (!ok || typeByte != static_cast<uint8_t>(NBTTagType::Compound)) return NBTCompound();
    [[maybe_unused]] std::string name = readModifiedUTF8(data, size, offset, ok);
    if (!ok) return NBTCompound();
    auto compound = readCompoundPayload(data, size, offset, ok);
    NBTCompound result;
    result.tags = std::move(compound);
    return result;
}

} // namespace VoxelForge
