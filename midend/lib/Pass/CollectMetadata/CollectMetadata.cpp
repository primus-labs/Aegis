#include <iostream>
#include <memory>

#include "mlir/IR/Attributes.h"
#include "llvm/ADT/StringRef.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "mlir/include/mlir/Support/LLVM.h" 
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "Common/MetadataMgr.h"
#include "Common/Utils.h"
#include "Pass/CollectMetadata/CollectMetadata.h"

using namespace mlir;
using namespace aegis;


void CollectMetadataPass::collectAllMetadata(func::FuncOp funcOp) 
{
    MetadataMgr &metaMgr = MetadataMgr::getInstance();

    for (unsigned i = 0; i < funcOp.getNumArguments(); ++i) {
        // Get name attribute and value
        llvm::StringRef nameAttrVal;
        if (auto nameAttr = funcOp.getArgAttr(i, PARAM_ATTR_NAME)) {
            if (auto strAttr = mlir::dyn_cast<StringAttr>(nameAttr)) {
                nameAttrVal = strAttr.getValue();
                //llvm::outs() << funcOp.getName() << " function argument " << i << "'th (" << PARAM_ATTR_NAME << "," << nameAttrVal << ")\n";
            }
        }

        // Get type attribute and value then save
        if (auto typeAttr = funcOp.getArgAttr(i, PARAM_ATTR_TYPE)) {
            if (auto strAttr = mlir::dyn_cast<StringAttr>(typeAttr)) {
                //llvm::outs() << funcOp.getName() << " function argument " << i << "'th (" << PARAM_ATTR_TYPE << "," << strAttr.getValue() << ")\n";
                metaMgr.addMetadata(nameAttrVal, strAttr.getValue());
            }
        }
    }
}

void CollectMetadataPass::runOnOperation() 
{
    auto moduleOp = getOperation();
    moduleOp.walk([this](func::FuncOp funcOp) {
        collectAllMetadata(funcOp);
    });
}