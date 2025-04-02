import primus_aegis

print("primus_aegis.__doc__:\n", primus_aegis.__doc__)
print("primus_aegis.__dict__:\n", primus_aegis.__dict__)

from primus_aegis.fhe import KeyInfo as FHEKeyInfo

print("FHEKeyInfo:\n", FHEKeyInfo.__dict__)

# make keyInfo (load from program_spec.json)
fheKeyInfo = FHEKeyInfo()
# set the fields according to program_spec.json, such as:
fheKeyInfo.multDepth = 1
fheKeyInfo.scaleModSize = 50
fheKeyInfo.batchSize = 8
fheKeyInfo.galoisIndices = [1, -2]

from primus_aegis.fhe import Keyset as FHEKeyset

print("FHEKeyset:\n", FHEKeyset.__dict__)

# generate keys
fheKeyset = FHEKeyset.getInstance()
fheKeyset.generate(key_info=fheKeyInfo)
fheKeyset.generate(key_info=fheKeyInfo)  # no effect
fheKeyset.generate(key_info=fheKeyInfo)  # no effect too


# serialize all keys(and context) to a string
all_keys = fheKeyset.to_bytes()
print("type of all_keys:", type(all_keys))
print("len of all_keys:", len(all_keys))

# also can only dump publicly information by setting `contain_sk` to False.
# and send to compute node
pub_keys = fheKeyset.to_bytes(contain_sk=False)
print("type of pub_keys:", type(pub_keys))
print("len of pub_keys:", len(pub_keys))

# you can only export private key or public key
fhePrivateKey = fheKeyset.getPrivateKey()
fhePublicKey = fheKeyset.getPublicKey()
private_key = fhePrivateKey.to_bytes()
public_key = fhePublicKey.to_bytes()
assert isinstance(private_key, bytes), "Expected .to_bytes() return type to be '<class 'bytes'>'"
assert isinstance(public_key, bytes), "Expected .to_bytes() return type to be '<class 'bytes'>'"
print("len of private_key:", len(private_key))
print("len of public_key:", len(public_key))

fhePrivateKey.from_bytes(private_key)
fhePublicKey.from_bytes(public_key)

#
#
# input/output
import numpy as np
from primus_aegis.fhe import DataProcessor as FHEDataProcessor

# test 1
plainInputData = np.array([[1.1, 2.2, 3.3, 4.4], [5.5, 6.6, 7.7, 8.8]], dtype=np.float64)
print("plainInputData:", plainInputData)

privateData = FHEDataProcessor.privateInput(plainInputData)
print("len of privateData:", len(privateData))

outputData = FHEDataProcessor.processOutput(privateData)
print("len of outputData:", len(outputData))
print("outputData:", outputData)

# test 2
plainInputData = np.array([[1.1, 2.2], [3.3, 4.4], [5.5, 6.6], [7.7, 8.8]], dtype=np.float64)
print("plainInputData:", plainInputData)

privateData = FHEDataProcessor.privateInput(plainInputData)
print("len of privateData:", len(privateData))

outputData = FHEDataProcessor.processOutput(privateData)
print("len of outputData:", len(outputData))
print("outputData:", outputData)
