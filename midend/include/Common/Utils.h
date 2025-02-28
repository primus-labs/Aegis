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


// calc index for a memref with strided and offset data.
int64_t calcFlattenIndex(llvm::ArrayRef<int64_t> indices, llvm::ArrayRef<int64_t> strides, int64_t offset);

// calc a unflatten ndex for a memref with strided and offset data.
llvm::SmallVector<int64_t> calcUnflattenIndex(int64_t index, llvm::ArrayRef<int64_t> strides, int64_t offset);

} // namespace aegis
} // namespace mlir



#endif //COMMON_UTILS_H