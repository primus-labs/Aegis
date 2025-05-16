#! /bin/bash

SCRIPT_DIR=$(cd `dirname $0` && pwd)
export MLIR_TRANSLATE_PATH=${SCRIPT_DIR}/../third_party/llvm-project/build/bin

SO_DIR=${SCRIPT_DIR}/../build/lib/Binding
FRONTEND_DIR=${SCRIPT_DIR}/../frontend

rm -rf ${SO_DIR}/data
cp -rf ${FRONTEND_DIR}/* ${SO_DIR}/


cd ${SO_DIR}

python3 test_1_compile.py
python3 test_2_keygen.py
python3 test_3_encrypt.py
python3 test_4_run.py
python3 test_5_decrypt.py
python3 test_6_local.py
