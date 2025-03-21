#!/bin/bash
set -e

cd ../build/bin
rm -rf ./temp
mkdir -p ./temp

# basic test
./fhe_test

# simulators
./sim_client1
./sim_server
./sim_client2
