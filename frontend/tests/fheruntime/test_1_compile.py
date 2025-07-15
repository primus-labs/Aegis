from primus.aegis.fheruntime import FHEServer as Server, FHEInferenceSession as InferenceSession, CompileOption, CompileResult, COMPILE_TARGET 
import numpy as np
import os
from test_config import aegis_onnx_file_path as onnx_file_path
from test_config import aegis_test_server_api as test_server_api
from test_config import aegis_is_sim as is_sim
from test_config import aegis_output_dir as output_dir

def add_2d_6elements(X1: list[list[float, 2], 3], X2: list[list[float, 2], 3]) -> list[list[float, 2], 3]:
    for i in range(3):
        for j in range(2):
            X1[i][j] = X1[i][j] + X2[i][j]
    return X1

def test_compile() -> str:
    print('test_server_api:', test_server_api)

    with open(onnx_file_path, 'rb') as f:
        onnx_file_bytes = f.read()
    param_annos = {'X1': 'encrypted', 'X2': 'clear'}
    compile_option = CompileOption()
    if is_sim:
        compile_option.compileTarget = COMPILE_TARGET.SIM_MLIR
    else:
        compile_option.compileTarget = COMPILE_TARGET.LIBRARY
    compile_option.outputDir = output_dir

    if test_server_api:
        server = Server(is_sim = is_sim)
        # compile_result = server.compile(onnx_file_path, param_annos, compile_option = compile_option)
        compile_result = server.compile(add_2d_6elements, param_annos, compile_option = compile_option)
        archive_path = server.save(compile_result, compile_result.get_output_dir_path())
    else:
        # inference_session = InferenceSession(onnx_file_path, param_annos, is_sim = is_sim, compile_option = compile_option)
        inference_session = InferenceSession(add_2d_6elements, param_annos, is_sim = is_sim, compile_option = compile_option)
        archive_path = inference_session.get_archive_path()
    return archive_path

if __name__ == '__main__':
    archive_path = test_compile()
    print('archive_path:', archive_path)
