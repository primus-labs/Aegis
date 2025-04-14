from primus_aegis.fhe import Keyset as FHEKeyset

print("Simulate Server")

# load keys and initialize context
fheKeyset = FHEKeyset.getInstance()
with open("pub_keys.bin", "rb") as f:
    pub_keys = f.read()
    fheKeyset.from_bytes(pub_keys)

    # for test
    if True:
        all_keys = fheKeyset.to_bytes()
        print("(test1) len of all_keys:", len(all_keys))
        pub_keys = fheKeyset.to_bytes(contain_sk=False)
        print("(test2) len of pub_keys:", len(pub_keys))


# got input from client
import numpy as np
from primus_aegis import Value
from primus_aegis.runtime import CompileResult, FHERuntime

with open("privateInputData.bin", "rb") as f:
    privateInputData = Value.from_bytes(f.read())

#
#
# ##########################
# (simulate) compile (this will be the first initial step of all)
from primus_aegis.runtime import CompileOption

mlir_file = ""
compileOption = CompileOption()

fheRuntime = FHERuntime()
compileResult = fheRuntime.compile(mlir_file, compileOption)
print("compileResult.outputDirPath:", compileResult.outputDirPath)
print("compileResult.cppFileName:", compileResult.cppFileName)
print("compileResult.binFileName:", compileResult.binFileName)
print("compileResult.progSpecFileName:", compileResult.progSpecFileName)
# ##########################
#
#

# runtime/run
fheRuntime = FHERuntime()
resultData = fheRuntime.run(privateInputData, compileResult)
with open("resultData.bin", "wb") as f:
    f.write(resultData.to_bytes())
