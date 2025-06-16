#!/bin/bash
set -e

curdir=$(pwd)
pydir=${curdir}/../midend/lib/Binding/
sodir=${curdir}/../build/lib/Binding/
cp -f ${pydir}/*.py ${sodir}
cd ${sodir}

#
#
# prepare
export LD_LIBRARY_PATH=${curdir}/../third_party/openfhe/build/lib
export PATH=${curdir}/../third_party/llvm-project/build/bin:$PATH

#
#
# compile
for ((i = 0; i < 4; i++)); do
  export CASE_INDEX=$i
  python test_1_compile.py
  python test_2_generate_key.py
  python test_3_input.py
  python test_4_runtime.py
  python test_5_output.py
done

#
#
# export api doc
cd ${sodir}
modules=("primus_aegis" "primus_aegis.compiler" "primus_aegis.fhe" "primus_aegis.dataprocessor" "primus_aegis.runtime")
for module in "${modules[@]}"; do
  python3 -c "import primus_aegis; help(${module})" >${module}.txt
  if command -v pydoc3 >/dev/null; then
    pydoc3 -w ${module}
  fi
done

exit 0

#
# basic test
python test_binding.py

#
# simulate test
python sim_client1.py
python sim_server.py
python sim_client2.py

#
#
# export api doc
cd ${sodir}
modules=("primus_aegis" "primus_aegis.fhe" "primus_aegis.runtime")
for module in "${modules[@]}"; do
  python3 -c "import primus_aegis; help(${module})" >${module}.txt
  if command -v pydoc3 >/dev/null; then
    pydoc3 -w ${module}
  fi
done
