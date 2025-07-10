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
from primus_aegis.dataprocessor import FHEDataProcessor, SimDataProcessor
from test_cases import testInputs, testIsSim

for i in range(len(testInputs)):
    input = testInputs[i]
    plainInput_i = np.array(input["value"], dtype=np.float64)
    print(f"plainInput_{i}:", plainInput_i, type(plainInput_i))

    if testIsSim:
        privateInput_i = SimDataProcessor.publicInput(plainInput_i)
        print(f"type of publicInput_{i}:", type(privateInput_i))
    elif "type" in input.keys() and input["type"] == "clear":
        privateInput_i = FHEDataProcessor.publicInput(plainInput_i)
        print(f"type of publicInput_{i}:", type(privateInput_i))
    else:
        privateInput_i = FHEDataProcessor.privateInput(plainInput_i)
        print(f"type of privateInput_{i}:", type(privateInput_i))

    with open(f"privateInput_{i}.bin", "wb") as f:
        f.write(privateInput_i.to_bytes())

#
# TEST
isTest = True
isTest = False
if not isTest:
    exit(0)

if testIsSim:
    pass
else:
    print("Test Input/Output 1")
    plain = np.array([1.0, 2.0, 3.0, 4.0], dtype=np.float64)
    encrypted = FHEDataProcessor.privateInput(plain)
    decrypted = FHEDataProcessor.processOutput(encrypted)
    print("decrypted", len(decrypted), decrypted)

    print("Test Input/Output 2")
    plain = np.array([[1.0, 2.0, 3.0, 3.0], [8.0, 7.0, 6.0, 6.0], [5.0, 9.0, 1.0, 4.0]], dtype=np.float64)
    encrypted = FHEDataProcessor.privateInput(plain)
    decrypted = FHEDataProcessor.processOutput(encrypted)
    print("decrypted", len(decrypted), decrypted)

    print("Test Input/Output 3")
    plain = np.array([[1.0, 3.0], [5.0, 7.0], [9.0, 11.0]], dtype=np.float64)
    encrypted = FHEDataProcessor.privateInput(plain)
    decrypted = FHEDataProcessor.processOutput(encrypted)
    print("decrypted", len(decrypted), decrypted)

exit(3)
