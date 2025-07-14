#! /bin/bash

SCRIPT_DIR=$(cd `dirname $0` && pwd)
TEST_DIR=${SCRIPT_DIR}/../frontend/tests/fheruntime

FRONTEND_DIR=${SCRIPT_DIR}/../frontend

cd ${FRONTEND_DIR}

${SCRIPT_DIR}/build_wheel.sh
pip install --force-reinstall dist/aegis-0.1.0-py3-none-any.whl

cd ${TEST_DIR}

export AEGIS_OUTPUT_DIR=${TEST_DIR}/output
export AEGIS_PROG_SPEC_PATH=${AEGIS_OUTPUT_DIR}/prog_spec.json
export AEGIS_CLIENT_ZIP_PATH=${AEGIS_OUTPUT_DIR}/client.zip

export AEGIS_DATA_DIR=${TEST_DIR}/data
export AEGIS_ALL_KEYS_PATH=${AEGIS_DATA_DIR}/all_keys.bin
export AEGIS_EVA_KEYS_PATH=${AEGIS_DATA_DIR}/eva_keys.bin
export AEGIS_ONNX_FILE_PATH=${AEGIS_DATA_DIR}/add.onnx

export TEST_SERVER_API=1
# export TEST_SERVER_API=0
export IS_SIM=1
export IS_SIM=0

python3 test_1_compile.py
python3 test_2_keygen.py
python3 test_3_encrypt.py
python3 test_4_run.py
python3 test_5_decrypt.py
python3 test_6_local.py
python3 test_7_onnxruntime.py
