"""
Server Side
"""

#
# load publicly keys and initialize context
from primus_aegis.fhe import Keyset as FHEKeyset

fheKeyset = FHEKeyset.getInstance()
with open("pub_keys.bin", "rb") as f:
    pub_keys = f.read()
    fheKeyset.from_bytes(pub_keys)


#
#
from primus_aegis.compiler import CompileResult

# NOTE: The compileResult is the return value of Compiler.compile(), ref test_1_compile.py
# Should make sure the files are existed
if True:
    compileResult = CompileResult()
    compileResult.outputDirPath = "./"
    compileResult.cppFileName = "output.cpp"
    compileResult.binFileName = "libtest.so"
    compileResult.progSpecFileName = "prog_spec.json"


# load the input values
from primus_aegis import Value

with open("privateInput_a.bin", "rb") as f:
    privateInput_a = Value.from_bytes(f.read())
with open("privateInput_b.bin", "rb") as f:
    privateInput_b = Value.from_bytes(f.read())


# do calculation
from primus_aegis.runtime import FHERuntime

fheRuntime = FHERuntime()
resultDatas = fheRuntime.run([privateInput_a, privateInput_b], compileResult)
if len(resultDatas) > 0:
    with open("resultData0.bin", "wb") as f:
        f.write(resultDatas[0].to_bytes())
