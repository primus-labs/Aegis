from primus.aegis.fheruntime import FHEClient as Client, FHEServer as Server, CompileOption, CompileResult, COMPILE_TARGET 
import numpy as np

def compile_cpp_to_library(compileResult):
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

    print(cmd)
    result = subprocess.run(cmd, capture_output=True, text=True)
    if not os.path.exists(f"{compileResult.outputDirPath}/{compileResult.binFileName}"):
        print("stdout:\n", result.stdout)
        print("stderr:\n", result.stderr)
        exit(2)

    print("compileResult.outputDirPath:", compileResult.outputDirPath)
    print("compileResult.cppFileName:", compileResult.cppFileName)
    print("compileResult.binFileName:", compileResult.binFileName)
    print("compileResult.progSpecFileName:", compileResult.progSpecFileName)

def test_compile() -> CompileResult:
    server = Server()
    compile_option = CompileOption()
    compile_option.compileTarget = COMPILE_TARGET.CPP
    compile_option.outputDir = "./"

    # mlir_file = server.convert_onnx_to_mlir('data/add.onnx')
    # print(mlir_file)
    compile_result = server.compile('data/test.mlir', compile_option)
    compile_cpp_to_library(compile_result)
    print('outputDir', compile_result.outputDirPath)
    print('cppFileName', compile_result.cppFileName)
    print('binFileName', compile_result.binFileName)
    print('progSpecFileName', compile_result.progSpecFileName)
    return compile_result

def dump_compile_result(compile_result, file):
    import json
    j = json.loads('{}')
    j['outputDirPath'] = compile_result.outputDirPath
    j['cppFileName'] = compile_result.cppFileName
    j['binFileName'] = compile_result.binFileName
    j['progSpecFileName'] = compile_result.progSpecFileName

    content = json.dumps(j)
    with open(file, 'w') as f:
        f.write(content)

if __name__ == '__main__':
    compile_result = test_compile()
    dump_compile_result(compile_result, 'data/compile_result.json')
