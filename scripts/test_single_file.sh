#!/bin/sh

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"
#echo "Current directory: $(pwd)"
#set -x 

if [ $# -ne 1 ]; then
    echo "\e[1;31mError: Please provide an MLIR full file name.\e[0m"
    echo "Usage: $0 <filename>"
    exit 1
fi

# get command line
filename=$1
cmdline=$(grep "^// RUN:" "$filename" | sed -E 's#^// RUN: aegiscompiler ##; s# <.*##')

if [ -z "$cmdline" ]; then
    err_msg="Error: No valid command line from $filename"
    echo "$err_msg"
    exit 1
fi
#echo "get command line:$cmdline"


cd ../debug_build/bin
FILECHECK_TOOL="../../third_party/llvm-project/build/bin/FileCheck"

#./aegiscompiler --collect-metadata --unroll-loop-and-memory-opt --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse < "$1" | "$FILECHECK_TOOL" "$1"

output=$(./aegiscompiler $cmdline < "$1" | "$FILECHECK_TOOL" "$1" 2>&1) 
if [ -z "$output" ]; then
    echo "\033[1;34mPass\033[0m"
else
    echo "$output"
fi

#set +x