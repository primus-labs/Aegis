#! /bin/bash

build_wheel=${1:-notbuild}

SCRIPT_DIR=$(cd `dirname $0` && pwd)
TEST_DIR=${SCRIPT_DIR}/../frontend/tests/fheruntime

FRONTEND_DIR=${SCRIPT_DIR}/../frontend

cd ${FRONTEND_DIR}

if [ "$build_wheel" == "build" ]; then
    ${SCRIPT_DIR}/build_wheel.sh
    pip install --force-reinstall dist/aegis-0.1.0-py3-none-any.whl
fi

cd ${TEST_DIR}


rm -rf /tmp/aegis_output
rm -rf /tmp/aegis_runtime

mkdir -p /tmp/aegis_output
mkdir -p /tmp/aegis_runtime

test_server_api_set=(OFF ON)
is_sim_set=(OFF ON)

for test_server_api in ${test_server_api_set[@]}
do
    for is_sim in ${is_sim_set[@]}
    do
        export AEGIS_TEST_SERVER_API=${test_server_api}
        export AEGIS_IS_SIM=${is_sim}
        echo "test server api:${test_server_api}"
        echo "is sim: ${is_sim}"

        python3 test_1_compile.py
        python3 test_2_keygen.py
        python3 test_3_encrypt.py
        python3 test_4_run.py
        python3 test_5_decrypt.py
        python3 test_6_local.py
        python3 test_7_onnxruntime.py
    done
done
