#ifndef DOUBLE_SERIALIZER_H
#define DOUBLE_SERIALIZER_H

#include <vector>
#include <cstdint>

namespace mlir {
namespace aegis {

// Serialization API
// Serialize a single double value
std::vector<uint8_t> serialize(double value);

// Serialize a vector of doubles
std::vector<uint8_t> serialize(const std::vector<double>& values);

// Serialize a matrix of doubles
std::vector<uint8_t> serialize(const std::vector<std::vector<double>>& values);


// Deserialization API
// Deserialize a single double value
double deserializeToDouble(const std::vector<uint8_t>& bytes);

// Deserialize to a vector of doubles
std::vector<double> deserializeToVector(const std::vector<uint8_t>& bytes);

// Deserialize to a matrix of doubles
std::vector<std::vector<double>> deserializeToMatrix(const std::vector<uint8_t>& bytes);

} // namespace aegis
} // namespace mlir

#endif // DOUBLE_SERIALIZER_H
