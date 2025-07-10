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
    with open("compileResult.json", "r", encoding="utf-8") as f:
        compileResult = CompileResult.from_json(f.read())
        print("compileResult:", compileResult.to_json(2))


# load the input values
from primus_aegis import Value
from test_cases import testInputsLength, testIsSim

privateInputs = []
for i in range(testInputsLength):
    with open(f"privateInput_{i}.bin", "rb") as f:
        privateInput_i = Value.from_bytes(f.read())
        privateInputs.append(privateInput_i)


# do calculation
from primus_aegis.runtime import FHERuntime, SimRuntime

if testIsSim:
    from primus_aegis.dataprocessor import SimDataProcessor
    from test_cases import testOutputs

    # NOTE, do not serialize the return value, since simulate.run return plain<double> directly
    resultDatas = SimRuntime().run(privateInputs, compileResult)

    outputData = SimDataProcessor.processOutput(resultDatas[0])
    print("len of outputData:", len(outputData))
    print("outputData:", outputData, "expectValue:", testOutputs[0]["value"])

    exit(0)


resultDatas = FHERuntime().run(privateInputs, compileResult)
if len(resultDatas) > 0:
    with open("resultData0.bin", "wb") as f:
        f.write(resultDatas[0].to_bytes())
