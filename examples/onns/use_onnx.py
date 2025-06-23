from primus.aegis.fheruntime import LocalFHEInferenceSession as LocalInferenceSession
import numpy as np
import os

py_dir = os.path.dirname(os.path.abspath(__file__))
onnx_file = os.path.join(py_dir, "add.onnx")

data_1 = np.array([[1, 3], 
                   [5, 7], 
                   [9, 11]], 
                   dtype = np.float32)
data_2 = np.array([[2, 4], 
                   [6, 8], 
                   [10, 12]], 
                   dtype = np.float32)

inference_session = LocalInferenceSession(onnx_file)
output = inference_session.encrypt_run_decrypt(['Y'], {'X1': data_1, 'X2': data_2})
print(output)