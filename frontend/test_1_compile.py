from primus.aegis.fheruntime import FHEClient as Client, FHEServer as Server, FHEInferenceSession as InferenceSession, CompileOption, CompileResult, COMPILE_TARGET 
import numpy as np

def test_compile() -> CompileResult:
    compile_option = CompileOption()
    compile_option.compileTarget = COMPILE_TARGET.LIBRARY
    compile_option.outputDir = './'

    if False:
        server = Server()
        mlir_file = server.convert_onnx_to_mlir('data/add.onnx')
        print(mlir_file)
        archive_path = server.compile(mlir_file)
        # archive_path = server.compile('data/euclidean_distance.mlir', compile_option)
        # archive_path = server.compile('data/add.onnx.mlir', compile_option)
    else:
        inference_session = InferenceSession('data/add.onnx', compile_option)
        archive_path = inference_session.get_archive_path()
    print(archive_path)
    return archive_path

def save_compile_result(compile_result, file):
    content = compile_result.to_json()
    with open(file, 'w') as f:
        f.write(content)

if __name__ == '__main__':
    test_compile()
