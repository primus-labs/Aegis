import re


def apply_mlir(input_mlir: str) -> str:
    """
    Apply the following changes to `input_mlir`:
      - Delete the `krnl.entry_point` line, if exists
      - Replace krnl.global with memref.global and memref.get_global, if krnl.global exists

    Args:
      input_mlir: the content of input mlir

    Returns:
      str: the content of modified mlir
    """
    # Delete the `krnl.entry_point` line
    input_mlir = re.sub(r'\s*"krnl\.entry_point"\(\).*', "", input_mlir)

    # More flexible handling
    global_defs = []
    output_mlir = ""
    inputs = input_mlir.split("\n")
    for line in inputs:
        # Set a placeholder in front of func.func
        pattern = r"^\s*func.func\s*@.*$"
        x = re.match(pattern, line)
        if x:
            output_mlir += f"GLOBAL_DEFINES_PLACEHOLDER{line}\n"
            continue

        # NOTE: now only apply the following format
        # %... = "krnl.global"() {name = ..., ..., value = dense<...> : tensor<...> } : () -> memref<...>
        pattern = re.compile(
            r'^\s*(?P<var>%[0-9a-zA-Z]+)\s*=\s*"krnl\.global"\(\)\s*\{'
            r'\s*name\s*=\s*"(?P<name>[^"]+)",'
            r".*?"
            r"\s*value\s*=\s*(?P<value>dense<[^>]+>)\s*:\s*(?P<vtype>tensor<[^>]+>)\s*"
            r".*?\}"
            r"\s*:\s*\(\)\s*->\s*(?P<rtype>memref<[^>]+>).*$"
        )
        x = re.match(pattern, line)
        if x:
            var = x.group("var")
            name = x.group("name")
            value = x.group("value")
            vtype = x.group("vtype").replace("tensor", "memref")
            rtype = x.group("rtype")

            global_defs.append(f"  memref.global @{name} : {vtype} = {value}\n")
            output_mlir += f"    {var} = memref.get_global @{name} : {rtype}\n"
        else:
            # Double-checking for leaks
            pattern = r'^\s*(%[0-9a-zA-Z]+)\s*=\s*"krnl\.global"\(\)\s*\{.*$'
            x = re.match(pattern, line)
            if x:
                raise RuntimeError(f"This form of krnl.global is not handled: {line}")
            else:
                output_mlir += f"{line}\n"

    global_defines = "".join(global_defs)
    output_mlir = output_mlir.replace("GLOBAL_DEFINES_PLACEHOLDER", global_defines)

    while output_mlir.endswith("\n\n"):
        output_mlir = output_mlir[:-1]

    return output_mlir


def _test_apply_mlir():
    input_mlir = """module {
  func.func @main_graph() -> memref<10xi64> {
    %0 = "krnl.global"() {name = "constant_0", alignment = 4096: i64, shape = [10], value = dense<[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]> : tensor<10xi64>} : () -> memref<10xi64>
    %1 = "krnl.global"() {name = "constant_1", alignment = 4096: i64, shape = [10], value = dense<[1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.1]> : tensor<10xf32>} : () -> memref<10xf32>
    %2 = "krnl.global"() {name = "constant_2", alignment = 4096: i64, shape = [10], value = dense<[21, 22, 23, 24, 25, 26, 27, 28, 29, 30]> : tensor<10xi64>} : () -> memref<10xi64>
    return %2 : memref<10xi64>
  }
  "krnl.entry_point"() {func = @main_graph, numInputs = 0 : i32, numOutputs = 1 : i32, signature = "[in_sig]\00@[out_sig]\00"} : () -> ()
}
"""
    output_mlir = apply_mlir(input_mlir)

    expected_output_mlir = """module {
  memref.global @constant_0 : memref<10xi64> = dense<[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]>
  memref.global @constant_1 : memref<10xf32> = dense<[1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.1]>
  memref.global @constant_2 : memref<10xi64> = dense<[21, 22, 23, 24, 25, 26, 27, 28, 29, 30]>
  func.func @main_graph() -> memref<10xi64> {
    %0 = memref.get_global @constant_0 : memref<10xi64>
    %1 = memref.get_global @constant_1 : memref<10xf32>
    %2 = memref.get_global @constant_2 : memref<10xi64>
    return %2 : memref<10xi64>
  }
}
"""
    assert output_mlir == expected_output_mlir

    input_mlir = """#map = affine_map<(d0) -> (d0)>
module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "addmul_model"} {
  func.func @main_graph(%arg0: memref<?xf32> {onnx.dim_params = "0:batch_size", onnx.name = "input_x"}, %arg1: memref<?xf32> {onnx.dim_params = "0:batch_size", onnx.name = "input_y"}) -> (memref<?xf32> {onnx.dim_params = "0:batch_size", onnx.name = "output"}) attributes {llvm.emit_c_interface} {
    %c0 = arith.constant 0 : index
    %0 = "krnl.global"() {name = "constant_0", shape = [], value = dense<2.000000e+00> : tensor<f32>} : () -> memref<f32>
    %dim = memref.dim %arg1, %c0 : memref<?xf32>
    %alloc = memref.alloc(%dim) {alignment = 16 : i64} : memref<?xf32>
    affine.for %arg2 = 0 to #map(%dim) {
      %1 = affine.load %arg1[%arg2] : memref<?xf32>
      %2 = affine.load %0[] : memref<f32>
      %3 = arith.mulf %1, %2 : f32
      %4 = affine.load %arg0[%arg2] : memref<?xf32>
      %5 = arith.addf %4, %3 : f32
      affine.store %5, %alloc[%arg2] : memref<?xf32>
    }
    return %alloc : memref<?xf32>
  }
  "krnl.entry_point"() {func = @main_graph, numInputs = 2 : i32, numOutputs = 1 : i32, signature = "[    { \22type\22 : \22f32\22 , \22dims\22 : [-1] , \22name\22 : \22input_x\22 }\0A ,    { \22type\22 : \22f32\22 , \22dims\22 : [-1] , \22name\22 : \22input_y\22 }\0A\0A]\00@[   { \22type\22 : \22f32\22 , \22dims\22 : [-1] , \22name\22 : \22output\22 }\0A\0A]\00"} : () -> ()
}
"""
    output_mlir = apply_mlir(input_mlir)

    expected_output_mlir = """#map = affine_map<(d0) -> (d0)>
module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "addmul_model"} {
  memref.global @constant_0 : memref<f32> = dense<2.000000e+00>
  func.func @main_graph(%arg0: memref<?xf32> {onnx.dim_params = "0:batch_size", onnx.name = "input_x"}, %arg1: memref<?xf32> {onnx.dim_params = "0:batch_size", onnx.name = "input_y"}) -> (memref<?xf32> {onnx.dim_params = "0:batch_size", onnx.name = "output"}) attributes {llvm.emit_c_interface} {
    %c0 = arith.constant 0 : index
    %0 = memref.get_global @constant_0 : memref<f32>
    %dim = memref.dim %arg1, %c0 : memref<?xf32>
    %alloc = memref.alloc(%dim) {alignment = 16 : i64} : memref<?xf32>
    affine.for %arg2 = 0 to #map(%dim) {
      %1 = affine.load %arg1[%arg2] : memref<?xf32>
      %2 = affine.load %0[] : memref<f32>
      %3 = arith.mulf %1, %2 : f32
      %4 = affine.load %arg0[%arg2] : memref<?xf32>
      %5 = arith.addf %4, %3 : f32
      affine.store %5, %alloc[%arg2] : memref<?xf32>
    }
    return %alloc : memref<?xf32>
  }
}
"""
    assert output_mlir == expected_output_mlir


if __name__ == "__main__":
    _test_apply_mlir()
