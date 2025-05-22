from primus.aegis.fheruntime import LocalFHEInferenceSession as LocalInferenceSession, CompileOption, CompileResult, COMPILE_TARGET 
from typing import List
import numpy as np
import os

if __name__ == '__main__':
    onnx_file_path = os.getenv('AEGIS_ONNX_FILE_PATH')
    output_dir = os.getenv('AEGIS_OUTPUT_DIR')

    data_1 = np.array([10])
    data_2 = np.array([14])

    inference_session = LocalInferenceSession(onnx_file = onnx_file_path)
    output = inference_session.encrypt_run_decrypt([data_1, data_2])
    print(output[0])

