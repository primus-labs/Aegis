#!/bin/sh

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"
#echo $SCRIPT_DIR

if [ $# -ne 1 ]; then
    echo "\e[1;31mError: Please provide an c/cpp full file name.\e[0m"
    echo "Usage: $0 <filename>"
    exit 1
fi

# Detect available C++ compiler
COMPILER=$(which g++ 2>/dev/null || which clang++ 2>/dev/null)

if [ -z "$COMPILER" ]; then
    echo "\e[1;31mError: Neither g++ nor clang++ compiler found.\e[0m"
    exit 1
fi


# Set OpenFHE installation paths 
OPENFHE_INCLUDE="/usr/local/include/openfhe"
OPENFHE_INCLUDE2="$SCRIPT_DIR/../third_party/openfhe/src/pke/include"
OPENFHE_INCLUDE3="$SCRIPT_DIR/../third_party/openfhe/src/binfhe/include"
OPENFHE_INCLUDE4="$SCRIPT_DIR/../third_party/openfhe/src/core/include"
OPENFHE_INCLUDE5="$SCRIPT_DIR/../third_party/openfhe/build/src/core"
OPENFHE_INCLUDE6="$SCRIPT_DIR/../third_party/openfhe/third-party/cereal/include"
OPENFHE_LIB="/usr/local/lib"
OPENFHE_LIB2="$SCRIPT_DIR/../third_party/openfhe/build/lib"


# Compilation command with optimization flags
#set -x
$COMPILER -shared -fPIC -std=c++17 -O2 \
    -I"$OPENFHE_INCLUDE" \
    -I"$OPENFHE_INCLUDE2" \
    -I"$OPENFHE_INCLUDE3" \
    -I"$OPENFHE_INCLUDE4" \
    -I"$OPENFHE_INCLUDE5" \
    -I"$OPENFHE_INCLUDE6" \
    -L"$OPENFHE_LIB" \
    -L"$OPENFHE_LIB2" \
    -o libtest.so  \
    "$1"  \
    -lOPENFHEcore -lOPENFHEpke -lpthread
    
#set +x

# Check build status and display results
if [ $? -eq 0 ]; then
    echo "\033[1;34mPass\033[0m"
    rm libtest.so
    #file libtest.so | grep -E 'shared object|ELF'
    #echo -e "\nLibrary dependencies:"
    #ldd libtest.so | grep OpenFHE
else
    echo "\e[1;31mFailed\e[0m"
    exit 1
fi
