from primus.aegis.fheruntime import FHEServer as Server, FHEInferenceSession as InferenceSession, CompileOption, CompileResult, COMPILE_TARGET 
import numpy as np
import os

def test_compile() -> str:
    onnx_file_path = os.getenv('AEGIS_ONNX_FILE_PATH')
    test_server_api = os.getenv('TEST_SERVER_API') == '1'
    print('test_server_api:', test_server_api)

    with open(onnx_file_path, 'rb') as f:
        onnx_file_bytes = f.read()

    if test_server_api:
        server = Server()
        mlir_file = server.convert_onnx_to_mlir(onnx_file_path)
        print('mlir_file:', mlir_file)
        compile_result = server.compile(mlir_file)
        archive_path = server.save(compile_result, compile_result.outputDirPath)
    else:
        inference_session = InferenceSession(path_or_bytes = onnx_file_path)
        archive_path = inference_session.get_archive_path()
    return archive_path

if __name__ == '__main__':
    archive_path = test_compile()
    print('archive_path:', archive_path)
