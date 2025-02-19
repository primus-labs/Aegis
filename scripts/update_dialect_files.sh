#!/bin/sh

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"
echo "Script file directory: $(pwd)"

echo "****************************************************"
echo "**************     build aegis doc     *************"
echo "****************************************************"
cd ../
mkdir -p build
cd build
# cmake ../midend/ -DMLIR_DIR=../third_party/llvm-project/build/lib/cmake/mlir/
make mlir-doc


echo "****************************************************"
echo "*********    copy secret dialect docs     **********"
echo "****************************************************"
cd docs/Secret
echo "copy SecretDialect.md..."
cat SecretDialect.md SecretOps.md SecretTypes.md > ../../../docs/dev/dialects/SecretDialect.md

echo "****************************************************"
echo "**********    copy fhe dialect docs     ***********"
echo "****************************************************"
cd ../FHE
echo "copy FHEDialect.md..."
cat FHEDialect.md FHEOps.md FHETypes.md > ../../../docs/dev/dialects/FHEDialect.md


echo "copy done."


