#include "Common/DoubleSerializer.h"
#include <cstring>
#include <stdexcept>
#include <type_traits>

namespace mlir {
namespace aegis {

// Check if the system uses little-endian byte order
inline bool isLittleEndian() {
    static const uint32_t testValue = 0x01020304;
    static const bool result = (reinterpret_cast<const uint8_t*>(&testValue)[0] == 0x04);
    return result;
}

// Convert host byte order to big-endian (64-bit version)
inline uint64_t toBigEndian64(uint64_t value) {
    if (isLittleEndian()) {
        // Reverse byte order for little-endian systems
        return ((value & 0x00000000000000FFULL) << 56) |
               ((value & 0x000000000000FF00ULL) << 40) |
               ((value & 0x0000000000FF0000ULL) << 24) |
               ((value & 0x00000000FF000000ULL) << 8) |
               ((value & 0x000000FF00000000ULL) >> 8) |
               ((value & 0x0000FF0000000000ULL) >> 24) |
               ((value & 0x00FF000000000000ULL) >> 40) |
               ((value & 0xFF00000000000000ULL) >> 56);
    }
    return value; // No conversion needed for big-endian systems
}

// Convert big-endian to host byte order (64-bit version)
inline uint64_t fromBigEndian64(uint64_t value) {
    // Conversion from big-endian to host is same as host to big-endian
    return toBigEndian64(value);
}

// Convert host byte order to big-endian (32-bit version)
inline uint32_t toBigEndian32(uint32_t value) {
    if (isLittleEndian()) {
        // Reverse byte order for little-endian systems
        return ((value & 0x000000FF) << 24) |
               ((value & 0x0000FF00) << 8) |
               ((value & 0x00FF0000) >> 8) |
               ((value & 0xFF000000) >> 24);
    }
    return value; // No conversion needed for big-endian systems
}

// Convert big-endian to host byte order (32-bit version)
inline uint32_t fromBigEndian32(uint32_t value) {
    // Conversion from big-endian to host is same as host to big-endian
    return toBigEndian32(value);
}

// Serialize a single double
inline void serializeDouble(double value, std::vector<uint8_t> &buffer) {
    uint64_t temp;
    std::memcpy(&temp, &value, sizeof(value));
    temp = toBigEndian64(temp); // Convert to big-endian
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&temp);
    buffer.insert(buffer.end(), bytes, bytes + sizeof(temp));
}

// Deserialize a single double
inline double deserializeDouble(const uint8_t *&data) {
    uint64_t temp;
    std::memcpy(&temp, data, sizeof(temp));
    data += sizeof(temp);
    temp = fromBigEndian64(temp); // Convert from big-endian
    double result;
    std::memcpy(&result, &temp, sizeof(result));
    return result;
}

// Serialize uint32_t (for container sizes)
inline void serializeUint32(uint32_t value, std::vector<uint8_t> &buffer) {
    uint32_t beValue = toBigEndian32(value); // Convert to big-endian
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&beValue);
    buffer.insert(buffer.end(), bytes, bytes + sizeof(beValue));
}

// Deserialize uint32_t (for container sizes)
inline uint32_t deserializeUint32(const uint8_t *&data) {
    uint32_t value;
    std::memcpy(&value, data, sizeof(value));
    data += sizeof(value);
    return fromBigEndian32(value); // Convert from big-endian
}

// ==============================================
// Serialization functions implementation
// ==============================================

std::vector<uint8_t> serializeFormDouble(double value) {
    std::vector<uint8_t> buffer;
    serializeDouble(value, buffer);
    return buffer;
}

std::vector<uint8_t> serializeFormVectorDouble(const std::vector<double>& values) {
    std::vector<uint8_t> buffer;
    
    // Serialize element count
    serializeUint32(static_cast<uint32_t>(values.size()), buffer);
    
    // Serialize each element
    for (const double& value : values) {
        serializeDouble(value, buffer);
    }
    
    return buffer;
}

std::vector<uint8_t> serializeFormMatrixDouble(const std::vector<std::vector<double>>& values) {
    std::vector<uint8_t> buffer;
    
    // Serialize outer element count
    serializeUint32(static_cast<uint32_t>(values.size()), buffer);
    
    // Serialize each inner vector
    for (const auto& innerVec : values) {
        // Serialize inner vector size
        serializeUint32(static_cast<uint32_t>(innerVec.size()), buffer);
        
        // Serialize inner vector elements
        for (const double& value : innerVec) {
            serializeDouble(value, buffer);
        }
    }
    
    return buffer;
}

// ==============================================
// Deserialization functions implementation
// ==============================================

double deserializeToDouble(const std::vector<uint8_t>& bytes) {
    if (bytes.size() != sizeof(double)) {
        throw std::runtime_error("Invalid byte count for single double");
    }
    
    const uint8_t* data = bytes.data();
    return deserializeDouble(data);
}

std::vector<double> deserializeToVectorDouble(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < sizeof(uint32_t)) {
        throw std::runtime_error("Insufficient bytes for vector<double> header");
    }
    
    const uint8_t* data = bytes.data();
    const uint8_t* end = data + bytes.size();
    
    // Deserialize element count
    uint32_t count = deserializeUint32(data);
    
    // Validate data length
    size_t expectedSize = sizeof(uint32_t) + count * sizeof(double);
    if (bytes.size() != expectedSize) {
        throw std::runtime_error("Invalid byte count for vector<double>");
    }
    
    // Deserialize each element
    std::vector<double> result;
    result.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        result.push_back(deserializeDouble(data));
    }
    

    // Verify all data consumed
    if (data != end) {
        throw std::runtime_error("Extra bytes in vector<double> data");
    }
    
    return result;
}

std::vector<std::vector<double>> deserializeToMatrixDouble(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < sizeof(uint32_t)) {
        throw std::runtime_error("Insufficient bytes for vector<vector<double>> header");
    }
    
    const uint8_t* data = bytes.data();
    const uint8_t* end = data + bytes.size();
    
    // Deserialize outer element count
    uint32_t outerCount = deserializeUint32(data);
    
    std::vector<std::vector<double>> result;
    result.reserve(outerCount);
    
    // Deserialize each inner vector
    for (uint32_t i = 0; i < outerCount; ++i) {
        if (data + sizeof(uint32_t) > end) {
            throw std::runtime_error("Missing inner vector size in vector<vector<double>>");
        }
        
        // Deserialize inner vector size
        uint32_t innerCount = deserializeUint32(data);
        
        // Validate sufficient data
        if (data + innerCount * sizeof(double) > end) {
            throw std::runtime_error("Insufficient bytes for inner vector in vector<vector<double>>");
        }
        
        // Deserialize inner vector elements
        std::vector<double> innerVec;
        innerVec.reserve(innerCount);
        for (uint32_t j = 0; j < innerCount; ++j) {
            innerVec.push_back(deserializeDouble(data));
        }
        result.push_back(std::move(innerVec));
    }
    
    // Verify all data consumed
    if (data != end) {
        throw std::runtime_error("Extra bytes in vector<vector<double>> data");
    }
    
    return result;
}

} // namespace aegis
} // namespace mlir