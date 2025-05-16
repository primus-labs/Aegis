from primus.aegis.fheruntime import FHEClient as Client, FHEServer as Server, FHEInferenceSession as InferenceSession, CompileOption, CompileResult, COMPILE_TARGET 
import numpy as np
import os

def test_compile() -> CompileResult:
    compile_option = CompileOption()
    compile_option.compileTarget = COMPILE_TARGET.LIBRARY
    compile_option.outputDir = os.getenv('AEGIS_OUTPUT_DIR')
    onnx_file_path = os.getenv('AEGIS_ONNX_FILE_PATH')

    test_server_api = os.getenv('TEST_SERVER_API') == '1'
    print('test_server_api:', test_server_api)
    if test_server_api:
        server = Server()
        mlir_file = server.convert_onnx_to_mlir(onnx_file_path)
        print(mlir_file)
        archive_path = server.compile(mlir_file, compile_option)
    else:
        inference_session = InferenceSession(onnx_file_path, compile_option)
        archive_path = inference_session.get_archive_path()
    print(archive_path)
    return archive_path

def save_compile_result(compile_result, file):
    content = compile_result.to_json()
    with open(file, 'w') as f:
        f.write(content)

if __name__ == '__main__':
    test_compile()
