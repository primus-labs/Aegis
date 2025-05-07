"""
Client Side
"""

#
# load keys and initialize context
from primus_aegis.fhe import Keyset as FHEKeyset

fheKeyset = FHEKeyset.getInstance()
with open("all_keys.bin", "rb") as f:
    all_keys = f.read()
    fheKeyset.from_bytes(all_keys)

#
# decrypt the result
import numpy as np
from primus_aegis import Value
from primus_aegis.dataprocessor import FHEDataProcessor

with open("resultData0.bin", "rb") as f:
    resultData0 = Value.from_bytes(f.read())
    print("type of resultData0:", type(resultData0))
    outputData = FHEDataProcessor.processOutput(resultData0)
    print("len of outputData:", len(outputData))
    expectValue = [1.23 * 4.5678]  # see test_3_input.py
    print("outputData:", outputData, "expectValue:", expectValue)
