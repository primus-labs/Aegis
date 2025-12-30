#!/bin/bash
# set -e
set -euxo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

#
# Build the project
cd ${SCRIPT_DIR}
bash ./build.sh

#
# Copy executable and shared library files
cd ${SCRIPT_DIR}
bash ./copy_files_to_frontend.sh

#
# Build wheel
# pip install build
# apt install python3.10-venv
FRONTEND_DIR=${SCRIPT_DIR}/../frontend
cd ${FRONTEND_DIR}

python3 -m build

#
# Install wheel
# pip install dist/primus*.whl
