from primus_aegis.fhe import KeyInfo as FHEKeyInfo, Keyset as FHEKeyset

print("Simulate Client 1")

# make keyInfo (load from program_spec.json)
fheKeyInfo = FHEKeyInfo()
fheKeyInfo.multDepth = 1
fheKeyInfo.scaleModSize = 50
fheKeyInfo.batchSize = 8
fheKeyInfo.galoisIndices = [1, -2]


# generate keys
fheKeyset = FHEKeyset.getInstance()
fheKeyset.generate(key_info=fheKeyInfo)


# serialize keys(and context) to a string
all_keys = fheKeyset.to_bytes()
print("len of all_keys:", len(all_keys))
pub_keys = fheKeyset.to_bytes(contain_sk=False)
print("len of pub_keys:", len(pub_keys))

with open("all_keys.bin", "wb") as f:
    f.write(all_keys)
with open("pub_keys.bin", "wb") as f:
    f.write(pub_keys)

# test private input, and checked in client2.py
import numpy as np
from primus_aegis.fhe import DataProcessor as FHEDataProcessor

plainInputData = np.array([[1.1, 2.2, 3.3, 4.4], [5.5, 6.6, 7.7, 8.8]], dtype=np.float64)
print("plainInputData:", plainInputData)

privateData = FHEDataProcessor.privateInput(plainInputData)
print("len of privateData:", len(privateData))

with open("privateData.bin", "wb") as f:
    f.write(privateData)
