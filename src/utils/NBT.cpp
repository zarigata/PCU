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

} // namespace VoxelForge
