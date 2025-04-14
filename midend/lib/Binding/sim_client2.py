from primus_aegis.fhe import Keyset as FHEKeyset

print("Simulate Client 2")

# load keys and initialize context
fheKeyset = FHEKeyset.getInstance()
with open("all_keys.bin", "rb") as f:
    all_keys = f.read()
    fheKeyset.from_bytes(all_keys)

    # for test
    if True:
        all_keys = fheKeyset.to_bytes()
        print("(test1) len of all_keys:", len(all_keys))
        pub_keys = fheKeyset.to_bytes(contain_sk=False)
        print("(test2) len of pub_keys:", len(pub_keys))

# decrypt run_result
import numpy as np
from primus_aegis import Value
from primus_aegis.fhe import DataProcessor as FHEDataProcessor

with open("resultData.bin", "rb") as f:
    resultData = Value.from_bytes(f.read())
    outputData = FHEDataProcessor.processOutput(resultData)
    print("len of outputData:", len(outputData))
    print("outputData:", outputData)
