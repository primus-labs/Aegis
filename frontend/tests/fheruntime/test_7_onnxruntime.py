import onnxruntime as ort
import numpy as np

if __name__ == '__main__':
    onnx_file = 'data/add.onnx'

    with open(onnx_file, 'rb') as f:
        content = f.read()

    # inference_session = ort.InferenceSession(path_or_bytes = 'data/add.onnx')
    inference_session = ort.InferenceSession(path_or_bytes = content)
    inputs = inference_session.get_inputs()
    outputs = inference_session.get_outputs()
    input_name = [i.name for i in inputs]
    output_name = [o.name for o in outputs]
    
    print(input_name)
    print(output_name)

    x1 = np.array([10.0], dtype = np.float32)
    x2 = np.array([14.0], dtype = np.float32)
    output = inference_session.run(['Y', 'Y'], {'X1': x1, 'X2': x2})
    print(output)
