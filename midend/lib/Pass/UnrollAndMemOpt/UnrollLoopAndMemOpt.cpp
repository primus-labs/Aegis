#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <vector>
#include <queue>
#include <unordered_map>
#include <utility>

#include "llvm/include/llvm/ADT/STLExtras.h"          
#include "llvm/include/llvm/ADT/SmallVector.h"       
#include "llvm/include/llvm/ADT/TypeSwitch.h"         
#include "llvm/include/llvm/Support/Casting.h"        
#include "llvm/include/llvm/Support/ErrorHandling.h"  
#include "mlir/include/mlir/Support/LLVM.h"              
#include "mlir/include/mlir/Support/LogicalResult.h"              
#include "mlir/include/mlir/IR/ImplicitLocOpBuilder.h"   
#include "mlir/include/mlir/IR/Location.h"               
#include "mlir/include/mlir/IR/Types.h"                      
#include "mlir/include/mlir/IR/Visitors.h"
#include "mlir/include/mlir/IR/Value.h"   
#include "mlir/include/mlir/Dialect/MemRef/IR/MemRef.h"  
#include "mlir/include/mlir/Dialect/Arith/IR/Arith.h"   
#include "mlir/include/mlir/Dialect/SCF/IR/SCF.h"  
#include "mlir/include/mlir/Dialect/Affine/Analysis/AffineAnalysis.h"  
#include "mlir/include/mlir/Dialect/Affine/Analysis/LoopAnalysis.h"  
#include "mlir/include/mlir/Dialect/Affine/IR/AffineMemoryOpInterfaces.h" 
#include "mlir/include/mlir/Dialect/Affine/IR/AffineOps.h"  
#include "mlir/include/mlir/Dialect/Affine/LoopUtils.h"  
#include "mlir/include/mlir/Dialect/Affine/Utils.h"      
#include "mlir/include/mlir/Dialect/Func/IR/FuncOps.h"   

#include "Common/Utils.h"
#include "Pass/UnrollAndMemOpt/UnrollLoopAndMemOpt.h"


using namespace mlir;
using namespace aegis;


//A map key combining the 'raw' underlying memref with the flattened access index 
// (evaluating all affine maps), used in caches for the pass.
typedef std::pair<Value, uint64_t> FlattenMemrefAccess;

class FlattenMemrefAccessHash {
public:
    size_t operator() (const FlattenMemrefAccess &pair) const {
        return mlir::hash_value(pair.first) ^ std::hash<uint64_t>()(pair.second);
    }
};

// For each normalized input key, this store ops that write to the corresponding underlying memory location, 
// regardless of differences in the memref name or metadata.
typedef std::unordered_multimap<FlattenMemrefAccess, Operation *,
                                FlattenMemrefAccessHash> StoreMap;


// removeUnusedMemrefOps deletes an allocOp and all of its users, provided that they are all unused. 
// For example, a memref that only contains store operations and is never read from is considered unused. 
// If the memref is aliased and the alias is never read from, the memref is also considered unused.
static LogicalResult removeUnusedMemrefOps(memref::AllocOp allocOp) {
    std::vector<Operation *> memrefAliasOps;
    std::vector<Operation *> storeOps;

    // collect all users
    auto memref = allocOp.getMemref();
    std::queue<Operation *> users;
    for (auto user : memref.getUsers()) {
        users.push(user);
    }

    // find the all users op can be read?
    for (; !users.empty(); users.pop()) {
        Operation *user = users.front();
        if (auto storeOp = dyn_cast<affine::AffineStoreOp>(user)) {
            storeOps.push_back(storeOp);
            continue;
        }

        bool isRead = llvm::TypeSwitch<Operation &, bool>(*user)
                    .Case<memref::CollapseShapeOp, memref::ExpandShapeOp, memref::SubViewOp, memref::ReinterpretCastOp>(
                        [&](auto op) {
                            for (auto user : op.getResult().getUsers()) {
                                users.push(user);
                            }

                            memrefAliasOps.push_back(op);
                            return false;
                        })
                    .Case<memref::ExtractStridedMetadataOp>([&](auto op) {
                        for (auto user : op.getResults()[0].getUsers()) {
                            users.push(user);
                        }

                        memrefAliasOps.push_back(op);
                        return false;
                    })
                    .Default([&](Operation &) { return true; });

        if (isRead) {
            return failure();
        }
    }

    // erase the store ops, alias ops and the allocOp.
    for (auto op : storeOps) {
        op->erase();
    }

    for (auto op : memrefAliasOps) {
        op->erase();
    }

    allocOp->erase();
    return success();
}


// Extract a static access index from the MemRefAccess and 
// flatten it into a 1D index within the underlying address space.
static FailureOr<int64_t> FlattenAndGetIndex(affine::MemRefAccess access, Operation &op, MemRefType type) {
    auto accessIndices = extractStaticIndices(access);
    if (!accessIndices) {
        op.emitWarning() << "Could not flatten the access indices";
        return failure();
    }

    llvm::SmallVector<int64_t> castIndices;
    auto flattenIndexSet = accessIndices.value();
    for (auto ndx : flattenIndexSet) {
        castIndices.push_back((int64_t)ndx);
    }

    auto [strides, offset] = getStridesAndOffset(type);
    return calcFlattenIndex(castIndices, strides, offset);
}

static FailureOr<int64_t> getFlattenAccessIndex(affine::AffineWriteOpInterface writeOp) {
    affine::MemRefAccess storeAccess(writeOp);
    return FlattenAndGetIndex(storeAccess, *writeOp, llvm::cast<MemRefType>(writeOp.getMemRef().getType()));
}

static FailureOr<int64_t> getFlattenAccessIndex(affine::AffineReadOpInterface readOp) {
    affine::MemRefAccess loadAccess(readOp);
    return FlattenAndGetIndex(loadAccess, *readOp, llvm::cast<MemRefType>(readOp.getMemRef().getType()));
}


// Search backwards through the IR to locate the original memref referred to by the input. 
// This could either be a memref created by an alloc operation or a function (block) argument memref.
// If the memref is a block argument, then getDefiningOp returns null, and we can exit because the memref we have is the source.
// If the defining op is a memref.alloc, in which case we'd infinitely loop. So we break out by swapping in nullptr for the "defining op."
static Value findSourceMemRef(Value memRef) {
    Value srcMemRef = memRef;
    Operation *op = memRef.getDefiningOp();
    while (op != nullptr) {
        auto [value, newOp] = llvm::TypeSwitch<Operation &, std::pair<Value, Operation *>>(*op)
                    .Case<memref::ReinterpretCastOp, memref::SubViewOp, memref::ExtractStridedMetadataOp>(
                        [&](auto op) {
                            return std::make_pair(op.getSource(),
                                                op.getSource().getDefiningOp());
                        })
                    .Case<memref::AllocOp>([&](auto op) {
                        return std::make_pair(op.getMemref(), nullptr);
                    })
                    .Case<memref::ExpandShapeOp, memref::CollapseShapeOp>([&](auto op) {
                        return std::make_pair(op.getSrc(), op.getSrc().getDefiningOp());
                    })
                    .Default([&](Operation &) {
                        llvm_unreachable("Get the defining op from the value, but unknown the defining op");
                        return std::make_pair(nullptr, nullptr);
                    });

        op = newOp;
        srcMemRef = value;
    }

    return srcMemRef;
}

// Go through all ops between fromOp and toOp and add all stores to the storeMap.
static LogicalResult collectStoreMap(Operation *fromOp, Operation *toOp, StoreMap &storeMap) {
    for (Operation *op = fromOp; op != toOp; op = op->getNextNode()) {
        auto storeOp = dyn_cast<affine::AffineStoreOp>(op);
        if (!storeOp) {
            continue;
        }

        auto res = getFlattenAccessIndex(storeOp);
        if (failed(res)) {
            storeOp.emitWarning() << "Found storeOp with unflatten access index.";
            return failure();
        }
        
        int64_t storeAccessIndex = res.value();
        Value storeSrcMemref = findSourceMemRef(storeOp.getMemRef());
        FlattenMemrefAccess storeIndexKey = std::make_pair(storeSrcMemref, storeAccessIndex);
        storeMap.insert(std::make_pair(storeIndexKey, storeOp));
    }

    return success();
}


// For a given load op that is not contained in any loop, and whose access indices are statically constant, 
// find the last store op that stores to the corresponding memory location, and forward the stored value to the load. 
// If the load is loading from a function argument memref, then collapse sequences
// of subview/expand/collapse/etc so that the target load is loading directly from the argument memref.
static LogicalResult forwardStoreToLoad(
    affine::AffineReadOpInterface loadOp, std::vector<Operation *> &opsToErase, StoreMap &storeMap) {
    // If the memrefOp is defined by GetGlobalOp then return directly.
    auto loadMemRef = loadOp.getMemRef();
    Operation *loadDefiningOp = loadMemRef.getDefiningOp();
    if (loadDefiningOp && dyn_cast<memref::GetGlobalOp>(loadDefiningOp)) {
        return failure();
    }

    // Get loadOp flatten access index.
    auto res = getFlattenAccessIndex(loadOp);
    if (failed(res)) {
        loadOp.emitWarning() << "Found loadOp with unflatten access index.";
        return failure();
    }
    int64_t loadAccessIndex = res.value();
    Value loadSourceMemref = findSourceMemRef(loadMemRef);
    FlattenMemrefAccess loadIndexKey = {loadSourceMemref, loadAccessIndex};

    // storeMap is an index of all stores that impact the given index.
    // Retrieve the latest store operation that's before the load operation.
    std::optional<Operation *> storeOpOrNull;
    auto storeRes = storeMap.equal_range(loadIndexKey);
    for (auto it = storeRes.first; it != storeRes.second; ++it) {
        if ((it->second)->isBeforeInBlock(loadOp) &&
            (!storeOpOrNull.has_value() || (storeOpOrNull.value()->isBeforeInBlock(it->second)))) {
            storeOpOrNull = it->second;
        }
    }

    if (!storeOpOrNull.has_value()) {
        if (!isa<BlockArgument>(loadSourceMemref)) {
            loadOp.emitWarning() << "Store op is null; loadOp=" << loadOp << "; load access index=" << loadAccessIndex;
            return failure();
        }
        if (loadSourceMemref == loadMemRef) {
            return success();
        }

        // The load cannot be completely removed, but instead can be replaced
        // with a load from the original memref at the appropriate index.
        auto memrefType = llvm::cast<MemRefType>(loadSourceMemref.getType());
        const auto [endingStrides, endingOffset] = getStridesAndOffset(memrefType);

        ImplicitLocOpBuilder opBuilder(loadOp->getLoc(), loadOp);
        llvm::SmallVector<Value> indexValues;
        for (auto ndx : calcUnflattenIndex(loadAccessIndex, endingStrides, endingOffset)) {
            Value ndxValue = opBuilder.create<arith::ConstantOp>(opBuilder.getIndexAttr(ndx));
            indexValues.push_back(ndxValue);
        }
        auto newLoadOp = opBuilder.create<affine::AffineLoadOp>(loadSourceMemref, indexValues);
        loadOp.getValue().replaceAllUsesWith(newLoadOp.getValue());
        opsToErase.push_back(loadOp);
        return success();
    }

    // In this case, the stored value can be forwarded directly to the load.
    // Check if the two values have the same shape. This is needed for affine vector loads and stores.
    Value storeVal = cast<affine::AffineWriteOpInterface>(storeOpOrNull.value()).getValueToStore();
    if (storeVal.getType() != loadOp.getValue().getType()) {
        return failure();
    }

    // Replace the load value with the store value.
    loadOp.getValue().replaceAllUsesWith(storeVal);

    // We can't delete the StoreOp at this point, because we may not fully remove all the places that load from it. 
    // Leave it to a future pass to delete all stores to memory locations that have no corresponding loads.
    opsToErase.push_back(loadOp);
    return success();
}


void UnrollLoopAndMemOptPass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<
        mlir::affine::AffineDialect, mlir::arith::ArithDialect, 
        mlir::scf::SCFDialect, mlir::memref::MemRefDialect>();
}


void UnrollLoopAndMemOptPass::runOnOperation() {
    mlir::ModuleOp module = getOperation();

    for (mlir::func::FuncOp func : module.getOps<mlir::func::FuncOp>()) {
        // Hold an intermediate computation of getFlattenedAccessIndex to avoid
        // repeated computations of MemRefAccess::getAccessMap
        std::vector<Operation *> opsToErase;

        // Add any stores to the store map that are not contained in any for loops.
        StoreMap storeMap;
        Operation &start = *func->getRegion(0).getOps().begin();
        auto end = *func.getOps<func::ReturnOp>().begin();
        if (failed(collectStoreMap(&start, end.getOperation(), storeMap))) {
            func.emitError() << "Failed to collect store map";
            return signalPassFailure();
        }

        // Collect the positions of the operations before and after the outer for loop.
        auto outerLoops = func.getOps<affine::AffineForOp>();
        for (auto root : llvm::make_early_inc_range(outerLoops)) {
            auto prevNode = root->getPrevNode();
            auto nextNode = root->getNextNode();

            SmallVector<affine::AffineForOp> nestedLoops;
            affine::getPerfectlyNestedLoops(nestedLoops, root);
            nestedLoops[0].getBody(0)->walk<WalkOrder::PostOrder>([&](affine::AffineForOp forOp) {
                auto unrollFactor = affine::getConstantTripCount(forOp).value_or(std::numeric_limits<int>::max());
                if (failed(loopUnrollUpToFactor(forOp, unrollFactor))) {
                    return WalkResult::skip();
                }
                return WalkResult::advance();
            });

            auto unrollFactor = affine::getConstantTripCount(root).value_or(std::numeric_limits<int>::max());
            if (failed(loopUnrollUpToFactor(root, unrollFactor))) {
                return signalPassFailure();
            }

            // Collect the storeMap indexing all newly unrolled stores from the end of
            // the last loop to the end of the current loop.
            if (failed(collectStoreMap(prevNode, nextNode, storeMap))) {
                return signalPassFailure();
            }

            //  Walk all load's and perform store to load forwarding.
            func.walk<WalkOrder::PreOrder>([&](affine::AffineReadOpInterface loadOp) {
                if (loadOp->getParentOp() != nextNode->getParentOp() || nextNode->isBeforeInBlock(loadOp)) {
                    // Only iterate on the loads we just unravelled. Because we walk
                    // in pre-order, we can interrupt the walk at this point.
                    return WalkResult::interrupt();
                }

                if (loadOp->getParentOp() != prevNode->getParentOp() || loadOp->isBeforeInBlock(prevNode)) {
                    // Don't process any loads prev to the currently inspected block
                    // that failed to forward, though ideally there should be none.
                    return WalkResult::skip();
                }

                if (failed(forwardStoreToLoad(loadOp, opsToErase, storeMap))) {
                    return WalkResult::skip();
                }

                return WalkResult::advance();
            });

            // Erase all load op's whose results were replaced with store fwd'ed
            // ones.
            for (auto *op : opsToErase) {
                op->erase();
            }
            opsToErase.clear();
        }

        // At this point, all the loops are unrolled, and all the load ops
        // that were within those loops, that could have been forwarded,
        // have been forwarded. However, there may still be load ops that
        // originated outside of any for loop that can still be forwarded.
        // So we need another pass over those load ops. 
        auto remainingLoads = func.getOps<affine::AffineLoadOp>();
        for (auto loadOp : llvm::make_early_inc_range(remainingLoads)) {
            if (failed(forwardStoreToLoad(loadOp, opsToErase, storeMap))) {
                continue;
            }
        }
        for (auto *op : opsToErase) {
            op->erase();
        }
        opsToErase.clear();

        // Now clear any unused memrefs. This clears memrefs that are allocated
        // during the program and their users when the memref (and any aliases of it) are no longer used. 
        auto remainingAllocs = func.getOps<memref::AllocOp>();
        for (auto allocOp : llvm::make_early_inc_range(remainingAllocs)) {
            if (failed(removeUnusedMemrefOps(allocOp))) {
                continue;
            }
        }
    }
}