from primus.aegis.fheruntime import FHEServer as Server, FHEInferenceSession as InferenceSession, CompileOption, CompileResult, COMPILE_TARGET 
import numpy as np
import os

def add_2d_6elements(X1: list[list[float, 2], 3], X2: list[list[float, 2], 3]) -> list[list[float, 2], 3]:
    for i in range(3):
        for j in range(2):
            X1[i][j] = X1[i][j] + X2[i][j]
    return X1

def test_compile() -> str:
    onnx_file_path = os.getenv('AEGIS_ONNX_FILE_PATH')
    test_server_api = os.getenv('TEST_SERVER_API') == '1'
    print('test_server_api:', test_server_api)

    with open(onnx_file_path, 'rb') as f:
        onnx_file_bytes = f.read()
    param_annos = {'X1': 'encrypted', 'X2': 'clear'}

    if test_server_api:
        server = Server()
        compile_result = server.compile(onnx_file_path, param_annos)
        # compile_result = server.compile(add_2d_6elements, param_annos)
        archive_path = server.save(compile_result, compile_result.get_output_dir_path())
    else:
        inference_session = InferenceSession(onnx_file_path, param_annos)
        archive_path = inference_session.get_archive_path()
    return archive_path

if __name__ == '__main__':
    archive_path = test_compile()
    print('archive_path:', archive_path)
