#ifndef PASS_MULTIDIMLOAD_LOWERMULTIDIMLOAD
#define PASS_MULTIDIMLOAD_LOWERMULTIDIMLOAD

#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

/// Lowers multi-dimensional memory access operations (e.g., memref.load(%arg, %c0, %c1)) 
/// into ​​vectorized loading (vload)​​ and 1D memory accesses(e.g., memref.load(%arg, %c1).
/// e.g.: %4 = fhe.load(%arg0, %c0, %c1) : (!fhe.lweciphermat<3 x 2 x f32>, index, index) -> !fhe.lwecipher<f32>
/// after lowering:
/// %4 = fhe.vload(%arg0, %c0) :(!fhe.lweciphermat<3 x 2 x f32>, index) -> !fhe.lwecipher<2 x f32>
/// %5 = fhe.load(%4, %c1) :  (!fhe.lweciphermat<2 x f32>, index) -> !fhe.lwecipher<f32>
struct LowerMultiDimLoadPass : public mlir::PassWrapper<LowerMultiDimLoadPass, mlir::OperationPass<mlir::ModuleOp>> {
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final {
        return "lower-multi-dim-load"; 
    }
};

#endif // PASS_MULTIDIMLOAD_LOWERMULTIDIMLOAD
