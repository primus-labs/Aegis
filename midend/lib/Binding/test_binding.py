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

outputData = FHEDataProcessor.processOutput(privateData)
print("len of outputData:", len(outputData))
print("outputData:", outputData)

# test 2
plainInputData = np.array([[1.1, 2.2], [3.3, 4.4], [5.5, 6.6], [7.7, 8.8]], dtype=np.float64)
print("plainInputData:", plainInputData)

privateData = FHEDataProcessor.privateInput(plainInputData)

outputData = FHEDataProcessor.processOutput(privateData)
print("len of outputData:", len(outputData))
print("outputData:", outputData)

# compiler engine
from primus_aegis.runtime import BACKEND_TYPE, COMPILE_TARGET, CompileOption

print("BACKEND_TYPE:", BACKEND_TYPE.CPU, ".value", BACKEND_TYPE.CPU.value)
print("BACKEND_TYPE:", BACKEND_TYPE.GPU, ".value", BACKEND_TYPE.GPU.value)
print("COMPILE_TARGET:", COMPILE_TARGET.SECRET, ".value", COMPILE_TARGET.SECRET.value)
print("COMPILE_TARGET:", COMPILE_TARGET.FHE, ".value", COMPILE_TARGET.FHE.value)
print("COMPILE_TARGET:", COMPILE_TARGET.EMITC, ".value", COMPILE_TARGET.EMITC.value)
print("COMPILE_TARGET:", COMPILE_TARGET.CPP, ".value", COMPILE_TARGET.CPP.value)
print("COMPILE_TARGET:", COMPILE_TARGET.LIBRARY, ".value", COMPILE_TARGET.LIBRARY.value)

compileOption = CompileOption()
print("compileOption.backendType:", compileOption.backendType)
print("compileOption.compileTarget:", compileOption.compileTarget)

compileOption.compileTarget = COMPILE_TARGET.EMITC
print("compileOption.backendType:", compileOption.backendType)
print("compileOption.compileTarget:", compileOption.compileTarget)

# runtime/compile
from primus_aegis.runtime import FHERuntime

mlir_file = ""
fheRuntime = FHERuntime()
compileResult = fheRuntime.compile(mlir_file, compileOption)
print("compileResult.outputDirPath:", compileResult.outputDirPath)
print("compileResult.cppFileName:", compileResult.cppFileName)
print("compileResult.binFileName:", compileResult.binFileName)
print("compileResult.progSpecFileName:", compileResult.progSpecFileName)


# runtime/run
plainInputData = np.array([[1.21, 2.12], [33.3, 41.4], [57.5, 6.66], [73.7, 18.8]], dtype=np.float64)
print("plainInputData:", plainInputData)

privateData = FHEDataProcessor.privateInput(plainInputData)
print("privateData:", type(privateData))
run_result = fheRuntime.run(privateData, compileResult)

# get the plain result
plain_result = FHEDataProcessor.processOutput(run_result)
print("plain_result:", plain_result)
