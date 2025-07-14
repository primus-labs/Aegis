
## Examples

- Deployment
  - [deploy/deployment.py](./deploy/deployment.py), full process of deployment.
  
- ONNX Model
  - [onnx/use_onnx.py](./onnx/use_onnx.py)
    This example demonstrates Aegis' support for ONNX models: It seamlessly compiles plaintext ONNX models into executable ciphertext models while maintaining full API compatibility with ONNX Runtime.
  - [onnx/use_onnx_sim.py](./onnx/use_onnx_sim.py)
    To verify the consistency between encrypted and plaintext execution of ONNX models, Aegis provides a simulate feature that executes models in plaintext mode. This example demonstrates how to use the simulate feature for ONNX model execution.
  
- ​Target Function
  - [loop/loop_2d_add.py](./loop/loop_2d_add.py)
    This example illustrates that Aegis supports loops, though the loop's induction variable must be constant.

  - [div/div.py](./div/div.py)
    This example demonstrates that Aegis supports ciphertext division operations.

  - [compare/apply_discount.py](./compare/apply_discount.py)
    This example demonstrates that Aegis supports ciphertext comparative operations.

  - [cipher_vs_plain/mix.py](./cipher_vs_plain/mix.py)
    This example demonstrates how Aegis supports mixed operations between ciphertexts and plaintexts.


### Usage

See [Installation](../README.md#installation) to set up the envirnoment.

Go to the example folder and execute `python3 xxx.py`.

