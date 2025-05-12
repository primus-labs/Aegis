from primus.aegis.fheruntime import FHEClient as Client, FHEServer as Server, FHEInferenceSession as InferenceSession, CompileOption, CompileResult, COMPILE_TARGET 
import numpy as np

def test_compile() -> CompileResult:
    compile_option = CompileOption()
    compile_option.compileTarget = COMPILE_TARGET.LIBRARY
    compile_option.outputDir = "./"

    if True:
        server = Server()
        mlir_file = server.convert_onnx_to_mlir('data/add.onnx')
        print(mlir_file)
        compile_result = server.compile(mlir_file, compile_option)
        # compile_result = server.compile('data/euclidean_distance.mlir', compile_option)
        # compile_result = server.compile('data/add.onnx.mlir', compile_option)
    else:
        inference_session = InferenceSession('data/add.onnx')
        compile_result = inference_session.get_compile_result()
    print("compileResult.outputDirPath:", compile_result.outputDirPath)
    print("compileResult.cppFileName:", compile_result.cppFileName)
    print("compileResult.binFileName:", compile_result.binFileName)
    print("compileResult.progSpecFileName:", compile_result.progSpecFileName)
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
