#include <iostream>
#include <memory>

#include "Common/MetadataMgr.h"
#include "Common/Utils.h"
#include "Pass/CollectMetadata/CollectMetadata.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/include/mlir/Support/LLVM.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/include/llvm/Support/Debug.h"

using namespace mlir;
using namespace aegis;

void CollectMetadataPass::collectAllMetadata(func::FuncOp funcOp) {
  MetadataMgr &metaMgr = MetadataMgr::getInstance();

  for (unsigned i = 0; i < funcOp.getNumArguments(); ++i) {
    // Get name attribute and value
    llvm::StringRef nameAttrVal;
    if (auto nameAttr = funcOp.getArgAttr(i, PARAM_ATTR_NAME)) {
      if (auto strAttr = mlir::dyn_cast<StringAttr>(nameAttr)) {
        nameAttrVal = strAttr.getValue();
        // llvm::outs() << funcOp.getName() << " function argument " << i <<
        // "'th (" << PARAM_ATTR_NAME << "," << nameAttrVal << ")\n";
      }
    }

    // Get type attribute and value then save
    if (auto typeAttr = funcOp.getArgAttr(i, PARAM_ATTR_TYPE)) {
      if (auto strAttr = mlir::dyn_cast<StringAttr>(typeAttr)) {
        // llvm::outs() << funcOp.getName() << " function argument " << i <<
        // "'th (" << PARAM_ATTR_TYPE << "," << strAttr.getValue() << ")\n";
        metaMgr.addMetadata(nameAttrVal, strAttr.getValue());
      }
    }
  }
}

void CollectMetadataPass::runOnOperation() {
    // Collect metadata
    auto moduleOp = getOperation();
    moduleOp.walk([this](func::FuncOp funcOp) { collectAllMetadata(funcOp); });

    // Create onnx.dims attribute for parameters & results
    auto processElements = [](func::FuncOp funcOp, bool isArguments) -> void {
        MLIRContext *context = funcOp.getContext();
        const size_t numElements = isArguments ? funcOp.getNumArguments() : funcOp.getNumResults();

        SmallVector<DictionaryAttr> newAttrsList;
        for (unsigned i = 0; i < numElements; ++i) {
            // Collect existing attributes
            DictionaryAttr existingAttrs = isArguments ? funcOp.getArgAttrDict(i) : funcOp.getResultAttrDict(i);
            SmallVector<NamedAttribute> attrs;
            if (existingAttrs) {
                attrs.append(existingAttrs.begin(), existingAttrs.end());
            }

            Type elementType =
                isArguments ? funcOp.getFunctionType().getInput(i) : funcOp.getFunctionType().getResult(i);

            // Process shaped types
            if (auto shapedType = mlir::dyn_cast<ShapedType>(elementType)) {
                if (shapedType.hasStaticShape()) {
                    SmallVector<Attribute> dimAttrs;
                    for (int64_t dim : shapedType.getShape()) {
                        dimAttrs.push_back(IntegerAttr::get(IntegerType::get(context, 64), dim));
                    }
                    attrs.push_back(
                        NamedAttribute(StringAttr::get(context, DIMS_ATTR_NAME), ArrayAttr::get(context, dimAttrs)));
                }
            } else {
                // Handle non-shaped types (scalars) by defaulting to dim=1
                SmallVector<Attribute> dimAttrs;
                dimAttrs.push_back(IntegerAttr::get(IntegerType::get(context, 64), 1));
                attrs.push_back(
                        NamedAttribute(StringAttr::get(context, DIMS_ATTR_NAME), ArrayAttr::get(context, dimAttrs)));
            }

            //Store updated attributes
            newAttrsList.push_back(DictionaryAttr::get(context, attrs));
        }

        // Update function attributes
        if (isArguments) {
            funcOp.setAllArgAttrs(newAttrsList);
        } else {
            funcOp.setAllResultAttrs(newAttrsList);
        }
    };

    moduleOp.walk([&](func::FuncOp funcOp) {
        // Prepare new argument attributes for all parameters
        MLIRContext* context = &getContext();
        processElements(funcOp, /*isArguments=*/true);
        processElements(funcOp, /*isArguments=*/false);

        return mlir::WalkResult::advance();
    });
}