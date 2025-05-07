"""
Development/Server Side
"""

from primus_aegis.compiler import COMPILE_TARGET, CompileOption, Compiler

# calculation: a * b
mlirContent = """
module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %5 = arith.mulf %arg0, %arg1 :  f32
        return %5 : f32
    }
}
"""

compileOption = CompileOption()
compileOption.compileTarget = COMPILE_TARGET.CPP
compileOption.outputDir = "./"
print("compileOption.backendType:", compileOption.backendType)
print("compileOption.compileTarget:", compileOption.compileTarget)

compileResult = Compiler().compile(mlirContent, compileOption)
if compileOption.compileTarget == COMPILE_TARGET.LIBRARY:
    print("compileResult.outputDirPath:", compileResult.outputDirPath)
    print("compileResult.cppFileName:", compileResult.cppFileName)
    print("compileResult.binFileName:", compileResult.binFileName)
    print("compileResult.progSpecFileName:", compileResult.progSpecFileName)


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

    print("compileResult.outputDirPath:", compileResult.outputDirPath)
    print("compileResult.cppFileName:", compileResult.cppFileName)
    print("compileResult.binFileName:", compileResult.binFileName)
    print("compileResult.progSpecFileName:", compileResult.progSpecFileName)
