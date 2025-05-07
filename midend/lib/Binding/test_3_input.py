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
# private input (calculation: a * b. The arguments/shapes ref test_1_compile.py's mlirContent)
import numpy as np
from primus_aegis.dataprocessor import FHEDataProcessor

plainInput_a = np.array([1.23], dtype=np.float64)
print("plainInput_a:", plainInput_a)
privateInput_a = FHEDataProcessor.privateInput(plainInput_a)
print("type of privateInput_a:", type(privateInput_a))

plainInput_b = np.array([4.5678], dtype=np.float64)
print("plainInput_b:", plainInput_b)
privateInput_b = FHEDataProcessor.privateInput(plainInput_b)
print("type of privateInput_b:", type(privateInput_b))


with open("privateInput_a.bin", "wb") as f:
    f.write(privateInput_a.to_bytes())
with open("privateInput_b.bin", "wb") as f:
    f.write(privateInput_b.to_bytes())
