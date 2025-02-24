#ifndef PASS_GLOBALMEMREFREPLACE_GLOBALMEMREFREPLACE_H
#define PASS_GLOBALMEMREFREPLACE_GLOBALMEMREFREPLACE_H

#include "mlir/include/mlir/IR/BuiltinAttributes.h"   
#include "mlir/include/mlir/IR/BuiltinOps.h"          
#include "mlir/include/mlir/IR/BuiltinTypes.h"        
#include "mlir/include/mlir/IR/DialectRegistry.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/include/mlir/IR/SymbolTable.h"    
#include "mlir/include/mlir/IR/PatternMatch.h"      
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"


struct GlobalMemrefReplacePass : public mlir::PassWrapper<GlobalMemrefReplacePass, mlir::OperationPass<mlir::ModuleOp>>
{
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final
    {
        return "global-memref-replace";
    }
};


#endif //PASS_GLOBALMEMREFREPLACE_GLOBALMEMREFREPLACE_H