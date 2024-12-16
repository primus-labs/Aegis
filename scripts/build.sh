#!/bin/sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"
echo "Current directory: $(pwd)"

echo "****************************************************"
echo "**************     build llvm/mlir     *************"
echo "****************************************************"

cd ../third_party/llvm-project
mkdir -p build
cd build

cmake -G Ninja ../llvm \
-DLLVM_ENABLE_PROJECTS=mlir \
-DLLVM_BUILD_EXAMPLES=OFF \
-DLLVM_TARGETS_TO_BUILD=X86 \
-DCMAKE_BUILD_TYPE=Release \
-DLLVM_ENABLE_ASSERTIONS=OFF \
-DCMAKE_C_COMPILER=clang \
-DCMAKE_CXX_COMPILER=clang++ \
-DLLVM_ENABLE_LLD=ON \
-DLLVM_CCACHE_BUILD=OFF \
-DLLVM_INSTALL_UTILS=ON \
-DMLIR_INCLUDE_INTEGRATION_TESTS=OFF

cmake --build .



echo "****************************************************"
echo "**************     build onnx-mlir     *************"
echo "****************************************************"
cd ../../onnx-mlir
mkdir -p build
cd build
cmake --build . --config Release


echo "****************************************************"
echo "**************      build OpenFHE      *************"
echo "****************************************************"



echo "****************************************************"
echo "**************       build aegis       *************"
echo "****************************************************"
cd ../../../
mkdir -p build
cd build
cmake ../midend/ -DMLIR_DIR=../third_party/llvm-project/build/lib/cmake/mlir/
make -j8
