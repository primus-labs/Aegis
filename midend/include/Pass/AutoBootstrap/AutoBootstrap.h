#ifndef PASS_AUTOBOOTSTRAP_AUTOBOOTSTRAP_H
#define PASS_AUTOBOOTSTRAP_AUTOBOOTSTRAP_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/IR/Value.h"

using namespace mlir;

struct AutoBootstrapPass : public mlir::PassWrapper<AutoBootstrapPass, mlir::OperationPass<mlir::ModuleOp>> {
    void getDependentDialects(mlir::DialectRegistry &registry) const;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final { 
        return "auto-bootstrap"; 
    }

public:
    struct ValueCompare {
        bool operator()(const mlir::Value &a, const mlir::Value &b) const {
            return a.getAsOpaquePointer() < b.getAsOpaquePointer();
        }
    };

private:
    // Process individual multiplication operation
    void processMulOp(Operation *mulOp);

    // Get multiplication depth for a value
    unsigned getChainDepth(Value val);

    // Insert bootstrap operation and update uses
    void insertBootstrapOp(Operation *insertAfter, Value val);

    // Identify fhe multiplication operations
    bool isMulOp(Operation *op);

    // Identify fhe comparison operations
    bool isCmpOp(Operation *op);

    // Identify fhe select operations
    bool isSelectOp(Operation *op);

    // Record where to insert bootstrap operation
    void recordInsertionPoint(Operation *op, Value val) {
        insertionPoints.emplace_back(op, val);
    }

    // Reset counter after bootstrapping
    void resetChainDepth(Value val) {
        valueChainDepth[val] = 0;
    }

private:
    // the fhe max mul depth
    unsigned int maxMulDepth;

    // Global tracking of multiplication chain depth for each value
    std::map<Value, unsigned, ValueCompare> valueChainDepth;
  
    // Record insertion points (value to bootstrap)
    std::vector<std::pair<Operation*, Value>> insertionPoints;
};

#endif  // PASS_AUTOBOOTSTRAP_AUTOBOOTSTRAP_H