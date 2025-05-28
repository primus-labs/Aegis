from primus.aegis.fheruntime import FHEServer as Server, FHEInferenceSession as InferenceSession, CompileOption, CompileResult, COMPILE_TARGET 
import numpy as np
import os

def add_1d_6elements(X1: list[float, 6], X2: list[float, 6]) -> list[float, 6]:
    for i in range(6):
        X1[i] = X1[i] + X2[i]
    return X1

def test_compile() -> str:
    onnx_file_path = os.getenv('AEGIS_ONNX_FILE_PATH')
    test_server_api = os.getenv('TEST_SERVER_API') == '1'
    print('test_server_api:', test_server_api)

    with open(onnx_file_path, 'rb') as f:
        onnx_file_bytes = f.read()

    if test_server_api:
        server = Server()
        # compile_result = server.compile(onnx_file = onnx_file_path)
        compile_result = server.compile(py_function = add_1d_6elements)
        archive_path = server.save(compile_result, compile_result.outputDirPath)
    else:
        inference_session = InferenceSession(path_or_bytes = onnx_file_path)
        archive_path = inference_session.get_archive_path()
    return archive_path

if __name__ == '__main__':
    archive_path = test_compile()
    print('archive_path:', archive_path)
