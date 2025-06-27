#!/bin/bash

build_type="Release"
if [[ $# -gt 0 ]]; then
  case "${1,,}" in
  "debug")
    build_type="Debug"
    ;;
  "release")
    build_type="Release"
    ;;
  *)
    echo "Unknown build type: $1"
    echo "Valid options: debug | release"
    exit 1
    ;;
  esac
fi

llvm_targets_to_build="X86"
if [[ $OSTYPE == 'darwin'* ]]; then
  llvm_targets_to_build="ARM;X86;AArch64"
fi
##########################################################

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"
echo "Current directory: $(pwd)"

echo "****************************************************"
echo "**************     build llvm/mlir     *************"
echo "****************************************************"

cd ../third_party/llvm-project
mkdir -p build
cd build
cmake -G Ninja ../llvm \
  -DLLVM_ENABLE_PROJECTS="mlir" \
  -DLLVM_BUILD_EXAMPLES=OFF \
  -DLLVM_TARGETS_TO_BUILD="${llvm_targets_to_build}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=OFF \
  -DLLVM_ENABLE_RTTI=ON \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DPython3_EXECUTABLE=$(which python3) \
  -DMLIR_ENABLE_BINDINGS_PYTHON=ON \
  -DLLVM_INSTALL_UTILS=ON \
  -DMLIR_INCLUDE_INTEGRATION_TESTS=OFF \
  -DMLIR_INCLUDE_TESTS=OFF
ninja -j8
sudo ninja install

echo "****************************************************"
echo " setup mlir core python bindings path to PYTHONPATH "
echo "****************************************************"
./../../../scripts/setup_pythonpath.sh

echo "****************************************************"
echo "**************     build onnx-mlir     *************"
echo "****************************************************"
MLIR_DIR=$(pwd)/lib/cmake/mlir
cd ../../onnx-mlir
mkdir -p build
cd build
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=OFF \
  -DONNX_MLIR_ENABLE_JAVA=OFF \
  -DMLIR_DIR=${MLIR_DIR} \
  ..
ninja -j8

echo "****************************************************"
echo "**************      build OpenFHE      *************"
echo "****************************************************"
cd ../../openfhe
mkdir -p build
cd build
cmake .. -DRUN_HAVE_POSIX_REGEX=0 \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_BENCHMARKS=OFF \
  -DBUILD_UNITTESTS=OFF \
  -DBUILD_EXAMPLES=OFF
make -j8
sudo make install

echo "****************************************************"
echo "**************       build aegis       *************"
echo "****************************************************"
cd ../../../
mkdir -p build
cd build
cmake ../midend/ \
  -DMLIR_DIR=../third_party/llvm-project/build/lib/cmake/mlir/ \
  -DCMAKE_BUILD_TYPE=${build_type} \
  -DPython3_EXECUTABLE=$(which python3)
make -j8

echo "****************************************************"
echo "**************       test aegis        *************"
echo "****************************************************"
make check-aegis
