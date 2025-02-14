#include "Common/Value.h"

#include <cstdint>
#include <stdlib.h>
#include <cstddef>
#include <sstream>

namespace mlir {
namespace aegis {


std::vector<size_t> Value::getDims() const {
    if (auto tensor = getTensor<int8_t>()) {
        return tensor.value().dims;
    } else if (auto tensor = getTensor<uint8_t>()) {
        return tensor.value().dims;
    } else if (auto tensor = getTensor<int16_t>()) {
        return tensor.value().dims;
    } else if (auto tensor = getTensor<uint16_t>()) {
        return tensor.value().dims;
    } else if (auto tensor = getTensor<int32_t>()) {
        return tensor.value().dims;
    } else if (auto tensor = getTensor<uint32_t>()) {
        return tensor.value().dims;
    } else if (auto tensor = getTensor<int64_t>()) {
        return tensor.value().dims;
    } else if (auto tensor = getTensor<uint64_t>()) {
        return tensor.value().dims;
    } else if (auto tensor = getTensor<float>()) {
        return tensor.value().dims;
    } else if (auto tensor = getTensor<double>()) {
        return tensor.value().dims;
    } else {
        assert(false);
    }
}

size_t Value::getLength() const {
    if (auto tensor = getTensor<int8_t>()) {
        return tensor.value().values.size();
    } else if (auto tensor = getTensor<uint8_t>()) {
        return tensor.value().values.size();
    } else if (auto tensor = getTensor<int16_t>()) {
        return tensor.value().values.size();
    } else if (auto tensor = getTensor<uint16_t>()) {
        return tensor.value().values.size();
    } else if (auto tensor = getTensor<int32_t>()) {
        return tensor.value().values.size();
    } else if (auto tensor = getTensor<uint32_t>()) {
        return tensor.value().values.size();
    } else if (auto tensor = getTensor<int64_t>()) {
        return tensor.value().values.size();
    } else if (auto tensor = getTensor<uint64_t>()) {
        return tensor.value().values.size();
    } else if (auto tensor = getTensor<float>()) {
        return tensor.value().values.size();
    } else if (auto tensor = getTensor<double>()) {
        return tensor.value().values.size();
    } else {
        assert(false);
    }
}

bool Value::isScalar() const {
    if (auto tensor = getTensor<int8_t>()) {
        return tensor.value().isScalar();
    } else if (auto tensor = getTensor<uint8_t>()) {
        return tensor.value().isScalar();
    } else if (auto tensor = getTensor<int16_t>()) {
        return tensor.value().isScalar();
    } else if (auto tensor = getTensor<uint16_t>()) {
        return tensor.value().isScalar();
    } else if (auto tensor = getTensor<int32_t>()) {
        return tensor.value().isScalar();
    } else if (auto tensor = getTensor<uint32_t>()) {
        return tensor.value().isScalar();
    } else if (auto tensor = getTensor<int64_t>()) {
        return tensor.value().isScalar();
    } else if (auto tensor = getTensor<uint64_t>()) {
        return tensor.value().isScalar();
    } else if (auto tensor = getTensor<float>()) {
        return tensor.value().isScalar();
    } else if (auto tensor = getTensor<double>()) {
        return tensor.value().isScalar();
    } else {
        assert(false);
    }
}

bool Value::operator==(const Value &x) const {
    if (auto tensor = getTensor<int8_t>()) {
        return tensor == x.getTensor<int8_t>();
    } else if (auto tensor = getTensor<uint8_t>()) {
        return tensor == x.getTensor<uint8_t>();
    } else if (auto tensor = getTensor<int16_t>()) {
        return tensor == x.getTensor<int16_t>();
    } else if (auto tensor = getTensor<uint16_t>()) {
        return tensor == x.getTensor<uint16_t>();
    } else if (auto tensor = getTensor<int32_t>()) {
        return tensor == x.getTensor<int32_t>();
    } else if (auto tensor = getTensor<uint32_t>()) {
        return tensor == x.getTensor<uint32_t>();
    } else if (auto tensor = getTensor<int64_t>()) {
        return tensor == x.getTensor<int64_t>();
    } else if (auto tensor = getTensor<uint64_t>()) {
        return tensor == x.getTensor<uint64_t>();
    } else if (auto tensor = getTensor<float>()) {
        return tensor == x.getTensor<float>();
    } else if (auto tensor = getTensor<double>()) {
        return tensor == x.getTensor<double>();    
    } else {
        assert(false);
    }
}

template <typename T>
std::string printTypeWithTensor(std::string type, Tensor<T> tensor) {
    std::stringstream str;

    if (tensor.isScalar()) {
        str << type << "(" << tensor.values[0] << ")";
    } else {
        str << type << "[](";
        for (auto v : tensor.values) {
            str << v << ",";
        }
        str << ")";
    }

    return str.str();
}

std::string Value::toString() const {
    if (auto tensor = getTensor<int8_t>(); tensor) {
        return printTypeWithTensor("int8_t", *tensor);
    } else if (auto tensor = getTensor<uint8_t>(); tensor) {
        return printTypeWithTensor("uint8_t", *tensor);
    } else if (auto tensor = getTensor<int16_t>(); tensor) {
        return printTypeWithTensor("int16_t", *tensor);
    } else if (auto tensor = getTensor<uint16_t>(); tensor) {
        return printTypeWithTensor("uint16_t", *tensor);
    } else if (auto tensor = getTensor<int32_t>(); tensor) {
        return printTypeWithTensor("int32_t", *tensor);
    } else if (auto tensor = getTensor<uint32_t>(); tensor) {
        return printTypeWithTensor("uint32_t", *tensor);
    } else if (auto tensor = getTensor<int64_t>(); tensor) {
        return printTypeWithTensor("int64_t", *tensor);
    } else if (auto tensor = getTensor<uint64_t>(); tensor) {
        return printTypeWithTensor("uint64_t", *tensor);
    } else if (auto tensor = getTensor<float>(); tensor) {
        return printTypeWithTensor("float", *tensor);
    } else if (auto tensor = getTensor<double>(); tensor) {
        return printTypeWithTensor("double", *tensor);
    } else {
        assert(false);
    }
}

} // namespace aegis
} // namespace mlir