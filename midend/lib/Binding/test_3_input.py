"""
Client Side
"""

#
# (IF NECCESSARY) load keys and initialize context
from primus_aegis.fhe import Keyset as FHEKeyset

fheKeyset = FHEKeyset.getInstance()
with open("all_keys.bin", "rb") as f:
    all_keys = f.read()
    fheKeyset.from_bytes(all_keys)


#
# private input (ref test_1_compile.py's mlirContent)
import numpy as np
from primus_aegis.dataprocessor import FHEDataProcessor
from test_cases import testInputs

for i in range(len(testInputs)):
    input = testInputs[i]
    plainInput_i = np.array(input["value"], dtype=np.float64)
    print(f"plainInput_{i}:", plainInput_i)

    privateInput_i = FHEDataProcessor.privateInput(plainInput_i)
    print(f"type of privateInput_{i}:", type(privateInput_i))

    with open(f"privateInput_{i}.bin", "wb") as f:
        f.write(privateInput_i.to_bytes())
