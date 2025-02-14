#ifndef COMMON_VALUE_H
#define COMMON_VALUE_H

#include "protocol.capnp.h"
#include "capnp/common.h"
#include "capnp/list.h"

#include <cstddef>
#include <cstdint>
#include <stdlib.h>
#include <vector>
#include <string>
#include <cassert>
#include <optional>
#include <initializer_list>
#include <variant>


namespace mlir {
namespace aegis {

template <typename T> 
struct Tensor {
    std::vector<T> values;
    std::vector<size_t> dims;

    /// Constructor
    Tensor<T>() = default;
    Tensor<T>(std::vector<T> values, std::vector<size_t> dims)
            : values(values), dims(dims) {}

    /// Constructor from initializer lists of values and dims.
    Tensor<T>(std::initializer_list<T> values,
            std::initializer_list<size_t> dims) {
        size_t cnt = 1;
        for (auto dim : dims) {
            cnt *= dim;
        }
        assert(values.size() == cnt);

        for (auto val : values) {
            this->values.push_back(val);
        }
        for (auto dim : dims) {
            this->dims.push_back(dim);
        }
    }

    /// Creates a tensor with the shape described by input dims, default filled with zeros.
    static Tensor<T> fromDims(std::vector<size_t> &dims) {
        uint32_t length = 1;
        for (auto dim : dims) {
            length *= dim;
        }
        auto values = std::vector<T>(length, 0);

        return Tensor{values, dims};
    }

    /// Conversion constructor from a value.
    Tensor<T>(T in) { 
        this->values.push_back(in); 
    }

    /// Operator Overload Function
    T &operator[](int index) { 
        return this->values[index]; 
    }

    bool operator==(const Tensor<T> &x) const {
        return this->values == x.values && this->dims == x.dims;
    }

    Tensor<T> operator+(T x) const {
        Tensor<T> res = *this;
        for (size_t i = 0; i < res.values.size(); i++) {
            res.values[i] += x;
        }
        return res;
    }

    Tensor<T> operator+(Tensor<T> x) const {
        assert(this->dims == x.dims);
        Tensor<T> res = *this;
        for (size_t i = 0; i < res.values.size(); i++) {
            res.values[i] += x.values[i];
        }
        return res;
    }

    Tensor<T> operator-(T x) const {
        Tensor<T> res = *this;
        for (size_t i = 0; i < res.values.size(); i++) {
            res.values[i] -= x;
        }
        return res;
    }

    Tensor<T> operator-(Tensor<T> x) const {
        assert(this->dims == x.dims);
        Tensor<T> res = *this;
        for (size_t i = 0; i < res.values.size(); i++) {
            res.values[i] -= x.values[i];
        }
        return res;
    }

    Tensor<T> operator*(T x) const {
        Tensor<T> res = *this;
        for (size_t i = 0; i < res.values.size(); i++) {
            res.values[i] *= x;
        }
        return res;
    }

    Tensor<T> operator*(Tensor<T> x) const {
        assert(this->dims == x.dims);
        Tensor<T> res = *this;
        for (size_t i = 0; i < res.values.size(); i++) {
            res.values[i] *= x.values[i];
        }
        return res;
    }

    template <typename U> 
    explicit operator Tensor<U>() const {
        Tensor<U> res;
        res.dims = this->dims;
        for (auto v : this->values) {
            res.values.push_back((U)v);
        }
        return res;
    }

    bool isScalar() const { return dims.empty(); }
};


/// A Value type for tensor data.
struct Value {

    std::variant<Tensor<uint8_t>, Tensor<int8_t>,
                 Tensor<uint16_t>, Tensor<int16_t>,
                 Tensor<uint32_t>, Tensor<int32_t>,
                 Tensor<uint64_t>, Tensor<int64_t>,
                 Tensor<float>, Tensor<double>>  data;

    /// Constructor
    Value() = default;
    Value(Tensor<uint8_t> data) : data(data){};
    Value(Tensor<int8_t> data) : data(data){};
    Value(Tensor<uint16_t> data) : data(data){};
    Value(Tensor<int16_t> data) : data(data){};
    Value(Tensor<uint32_t> data) : data(data){};
    Value(Tensor<int32_t> data) : data(data){};
    Value(Tensor<uint64_t> data) : data(data){};
    Value(Tensor<int64_t> data) : data(data){};
    Value(Tensor<float> data) : data(data){};
    Value(Tensor<double> data) : data(data){};

    std::vector<size_t> getDims() const;
    size_t getLength() const;
    std::string toString() const;

    template <typename T> 
    bool hasDataType() const {
        return std::holds_alternative<Tensor<T>>(data);
    }

    template <typename T> 
    std::optional<Tensor<T>> getTensor() const {
        if (!hasDataType<T>()) {
            return std::nullopt;
        }
        return std::get<Tensor<T>>(data);
    }

    template <typename T> 
    Tensor<T> *getTensorPtr() {
        if (!hasDataType<T>()) {
            return nullptr;
        }
        return &std::get<Tensor<T>>(data);
    }

    bool isScalar() const;

    /// Operator Overload == 
    bool operator==(const Value &x) const;
};


} // namespace aegis
} // namespace mlir


#endif