#!/bin/sh

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"
#echo "Current directory: $(pwd)"
#set -x 

if [ $# -ne 1 ]; then
    # echo "Error: Please provide an MLIR full file name."
    echo "\e[1;31mError: Please provide an MLIR full file name.\e[0m"
    exit 1
fi


cd ../debug_build/bin
FILECHECK_TOOL="../../third_party/llvm-project/build/bin/FileCheck"

#./aegiscompiler -collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse < "$1" | "$FILECHECK_TOOL" "$1"

output=$(./aegiscompiler -collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse < "$1" | "$FILECHECK_TOOL" "$1" 2>&1) 
if [ -z "$output" ]; then
    echo "\033[1;34mPass\033[0m"
else
    echo "$output"
fi

#set +x