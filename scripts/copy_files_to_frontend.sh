#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

#
#
primus_aegis_so=$(ls ${SCRIPT_DIR}/../build/lib/Binding/primus_aegis.cpython*.so)
aegis_rt_so=${SCRIPT_DIR}/../build/lib/Runtime/FHE/libAegisRuntime.so.20.0git
onnx_mlir_exe=${SCRIPT_DIR}/../third_party/onnx-mlir/build/Release/bin/onnx-mlir
mlir_translate_exe=${SCRIPT_DIR}/../third_party/llvm-project/build/bin/mlir-translate
for i in "${primus_aegis_so}" "${aegis_rt_so}" "${onnx_mlir_exe}" "${mlir_translate_exe}"; do
  if [ ! -f "${i}" ]; then
    echo "File ${i} not exist! Please run './build.sh' to generate it"
    exit 1
  fi
done

#
#
DIST_DIR=${SCRIPT_DIR}/../frontend/primus
mkdir -p ${DIST_DIR}/bin ${DIST_DIR}/lib
cp -f ${primus_aegis_so} ${DIST_DIR}/lib/
cp -f ${aegis_rt_so} ${DIST_DIR}/lib/
cp -f ${onnx_mlir_exe} ${DIST_DIR}/bin/
cp -f ${mlir_translate_exe} ${DIST_DIR}/bin/
chmod +x ${DIST_DIR}/bin/* ${DIST_DIR}/lib/*
