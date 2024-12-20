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
cmake ../midend/ -DMLIR_DIR=../third_party/llvm-project/build/lib/cmake/mlir/
make mlir-doc


echo "****************************************************"
echo "*************    copy dialect docs     *************"
echo "****************************************************"
cd docs/Secret
echo "copy SecretDialect.md..."
cat SecretDialect.md SecretOps.md SecretTypes.md > ../../../docs/dev/dialects/SecretDialect.md

#echo "copy FHEDialect.md..."
#cp FHEDialect.md ../../../docs/dev/dialects/

#echo "copy CKKSDialect.md..."
#cp CKKSDialect.md ../../../docs/dev/dialects/

echo "copy done."


