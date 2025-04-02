#!/bin/bash
set -e

curdir=$(pwd)
pydir=${curdir}/../midend/lib/Binding/
sodir=${curdir}/../build/lib/Binding/
cp -f ${pydir}/*.py ${sodir}
cd ${sodir}

#
# basic test
python test_binding.py

#
# simulate test
python sim_client1.py
python sim_server.py
python sim_client2.py
