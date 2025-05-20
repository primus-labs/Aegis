#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include "mlir/IR/Value.h"
#include "mlir/include/mlir/IR/BuiltinOps.h"
#include "mlir/include/mlir/Dialect/Affine/Analysis/AffineAnalysis.h"
#include "mlir/include/mlir/Dialect/Affine/Utils.h"
#include "mlir/include/mlir/IR/Types.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/include/llvm/ADT/ArrayRef.h"
#include "llvm/include/llvm/ADT/SmallVector.h"
#include <cstdint>
#include <optional>
#include <vector>

#define PARAM_ATTR_NAME "onnx.name"
#define PARAM_ATTR_TYPE "onnx.type"
#define ENCRYPTED "encrypted"
#define CLEAR "clear"
#define DIMS_ATTR_NAME  "onnx.dims"
#define GALOIS_KEY_INDEX    "fhe.GaloisKeyIndex"
#define EXPORT_FUNCNAME_PRIFIX  "aegis_mlir_"

#define FHE_MAX_MUL_DEPTH   18
#define FHE_FIRST_MOD_SIZE  60
#define FHE_SCALE_MOD_SIZE  50
inline bool enableFheBoostrapFlag = false;

namespace mlir {
namespace aegis {

// Extract statically constant input indices from a MemRefAccess to a vector of
// indexes per dimension. Returns a std::nullopt if the indices are not
// constants (e.g. derived from inputs).
std::optional<std::vector<uint64_t>>
extractStaticIndices(const affine::MemRefAccess &access);

// Extract and flatten the index of a MemRefAccess to the corresponding index in
// a 1-dimensional flattened array. Returns a std::nullopt if the indices are
// not constants (e.g. derived from inputs).
std::optional<uint64_t>
getFlattenedAccessIndex(const affine::MemRefAccess &access,
                        mlir::Type memRefType);

// calc index for a memref with strided and offset data.
int64_t calcFlattenIndex(llvm::ArrayRef<int64_t> indices,
                         llvm::ArrayRef<int64_t> strides, int64_t offset);

// calc a unflatten ndex for a memref with strided and offset data.
llvm::SmallVector<int64_t> calcUnflattenIndex(int64_t index,
                                              llvm::ArrayRef<int64_t> strides,
                                              int64_t offset);

// Helper function to check if a value is encrypted
bool isEncrypted(Value value, llvm::DenseMap<Value, bool> &cache);

// Helper function to check if a value(func params) is encrypted
bool isArgEncrypted(Value value);

// Appends an integer element to the GaloisIndex metadata attribute
void addGaloisIndex(mlir::ModuleOp module, int32_t newVal);

//  Retrieves all elements from GaloisIndex metadata attribute
llvm::SmallVector<int32_t> getAllGaloisIndexs(mlir::ModuleOp module);

} // namespace aegis
} // namespace mlir

#endif // COMMON_UTILS_H