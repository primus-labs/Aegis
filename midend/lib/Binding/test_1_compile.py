"""
Development/Server Side
"""

from primus_aegis.compiler import COMPILE_TARGET, CompileOption, Compiler
from test_cases import testMlirContent

mlirContent = testMlirContent
compileOption = CompileOption()
compileOption.compileTarget = COMPILE_TARGET.LIBRARY
compileOption.outputDir = "./"
print("compileOption:", compileOption.to_json(1))

compileResult = Compiler().compile(mlirContent, compileOption)
if compileOption.compileTarget == COMPILE_TARGET.LIBRARY:
    print("compileResult:", compileResult.to_json())
    with open("compileResult.json", "w", encoding="utf-8") as f:
        f.write(compileResult.to_json())

# Please install openfhe to system first,
# if you encounter "fatal error: openfhe.h: No such file or directory"

#
# THE IS NOT NECCESSARY, if you have installed the openfhe, so
# you can set compileOption.compileTarget = COMPILE_TARGET.LIBRARY to skip this
#
# compile cpp to .so
if compileOption.compileTarget == COMPILE_TARGET.CPP:
    import subprocess
    import os

    # NOTE: relate to build/lib/Binding
    openfhe_dir = "../../../third_party/openfhe"
    if not os.path.exists(openfhe_dir):
        print(f"{openfhe_dir} not exist!")
        exit(1)

    compileResult.binFileName = "libtest.so"

    include_dirs = [
        f"/usr/local/include/openfhe",
        f"{openfhe_dir}/src/pke/include",
        f"{openfhe_dir}/src/binfhe/include",
        f"{openfhe_dir}/src/core/include",
        f"{openfhe_dir}/build/src/core",
        f"{openfhe_dir}/third-party/cereal/include",
    ]
    lib_dirs = [
        f"/usr/local/lib",
        f"{openfhe_dir}/build/lib",
    ]
    libs = ["OPENFHEcore", "OPENFHEpke", "pthread"]
    cmd = ["g++", "-shared", "-fPIC", "-std=c++17", "-O2", "-o", compileResult.binFileName, compileResult.cppFileName]
    for inc in include_dirs:
        cmd += ["-I", inc]
    for lib in lib_dirs:
        cmd += ["-L", lib]
    for lib in libs:
        cmd += ["-l" + lib]

    result = subprocess.run(cmd, capture_output=True, text=True)
    if not os.path.exists(f"{compileResult.outputDirPath}/{compileResult.binFileName}"):
        print("stdout:\n", result.stdout)
        print("stderr:\n", result.stderr)
        exit(2)

    print("compileResult:", compileResult.to_json(2))
    with open("compileResult.json", "w", encoding="utf-8") as f:
        f.write(compileResult.to_json())
