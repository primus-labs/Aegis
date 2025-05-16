from primus.aegis.fheruntime import LocalFHEInferenceSession as LocalInferenceSession, CompileOption, CompileResult, COMPILE_TARGET 
from typing import List
import numpy as np

if __name__ == '__main__':
    data_1 = np.array([10])
    data_2 = np.array([14])

    compile_option = CompileOption()
    compile_option.compileTarget = COMPILE_TARGET.LIBRARY
    compile_option.outputDir = "./output"

    inference_session = LocalInferenceSession('data/add.onnx', compile_option)
    output = inference_session.encrypt_run_decrypt([data_1, data_2])
    print(output)

