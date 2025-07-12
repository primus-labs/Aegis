from primus.aegis.fheruntime import LocalFHEInferenceSession as LocalInferenceSession 
from typing import List
import numpy as np
import os

if __name__ == '__main__':
    onnx_file_path = os.getenv('AEGIS_ONNX_FILE_PATH')
    output_dir = os.getenv('AEGIS_OUTPUT_DIR')
    is_sim = os.getenv('IS_SIM') == '1'

    with open(onnx_file_path, 'rb') as f:
        onnx_file_bytes = f.read()
    param_annos = {'X1': 'encrypted', 'X2': 'clear'}

    data_1 = np.array([[1, 3], [5, 7], [9, 11]], dtype = np.float32)
    data_2 = np.array([[2, 4], [6, 8], [10, 12]], dtype = np.float32)

    # data_1 = np.array([1, 3, 5, 7, 9, 11], dtype = np.float32)
    # data_2 = np.array([2, 4, 6, 8, 10, 12], dtype = np.float32)

    inference_session = LocalInferenceSession(onnx_file_path, param_annos, is_sim = is_sim)
    output = inference_session.run(['Y'], {'X1' : data_1, 'X2': data_2})
    print(__file__, output)

