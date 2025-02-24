#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <cstdint>
#include <vector>
#include <optional>
#include "llvm/include/llvm/ADT/ArrayRef.h"     
#include "llvm/include/llvm/ADT/SmallVector.h"  
#include "mlir/include/mlir/IR/Types.h"   
#include "mlir/include/mlir/Dialect/Affine/Utils.h"     
#include "mlir/include/mlir/Dialect/Affine/Analysis/AffineAnalysis.h"          


namespace mlir {
namespace aegis {

// Extract statically constant input indices from a MemRefAccess to a vector of indexes per dimension. 
// Returns a std::nullopt if the indices are not constants (e.g. derived from inputs).
std::optional<std::vector<uint64_t>> extractStaticIndices(const affine::MemRefAccess &access);


// Extract and flatten the index of a MemRefAccess to the corresponding index in a 1-dimensional flattened array. 
// Returns a std::nullopt if the indices are not constants (e.g. derived from inputs).
std::optional<uint64_t> getFlattenedAccessIndex(const affine::MemRefAccess &access, mlir::Type memRefType);

} // namespace aegis
} // namespace mlir



#endif //COMMON_UTILS_H