import os

CASE_INDEX = int(os.environ.get("CASE_INDEX", 0))
# print("CASE INDEX:", CASE_INDEX)
TEST_CASES = [
    {
        "mlirContent": """
    module {
        func.func @main_graph(%arg0: f64 {onnx.name = "input_x", onnx.type = "encrypted"},
                              %arg1: f64 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f64 {
            %5 = arith.mulf %arg0, %arg1 :  f64
            return %5 : f64
        }
    }
    """,
        "inputs": [{"value": 1.23}, {"value": 4.5678}],
        "expectOutputs": [{"value": 5.618394}],
        "isSim": True,
    },
    {
        "mlirContent": """
    module {
      func.func @main_graph(%arg0: memref<6xf64>, %arg1: memref<6xf64>) -> memref<6xf64> {
        affine.for %arg2 = 0 to 6 {
          %0 = affine.load %arg0[%arg2] : memref<6xf64>
          %1 = affine.load %arg1[%arg2] : memref<6xf64>
          %2 = arith.addf %0, %1 : f64
          affine.store %2, %arg0[%arg2] : memref<6xf64>
        }
        return %arg0 : memref<6xf64>
      }
    }
    """,
        "inputs": [{"value": [1.1, 2.2, 3.3, 1.4, 2.4, 3.4]}, {"value": [4.4, 5.5, 6.6, 2.5, 3.5, 4.5]}],
        "expectOutputs": [{"value": [5.5, 7.7, 9.9, 3.9, 5.9, 7.9]}],
        "isSim": True,
    },
    {
        "mlirContent": """
    module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "add"} {
      func.func @main_graph(%arg0: memref<3x2xf64> {onnx.name = "X1", onnx.type = "encrypted"},
                            %arg1: memref<3x2xf64> {onnx.name = "X2", onnx.type = "encrypted"})
                            -> (memref<3x2xf64> {onnx.name = "Y"}) attributes {llvm.emit_c_interface} {
        %alloc = memref.alloc() {alignment = 16 : i64} : memref<3x2xf64>
        affine.for %arg2 = 0 to 3 {
          affine.for %arg3 = 0 to 2 {
            %0 = affine.load %arg0[%arg2, %arg3] : memref<3x2xf64>
            %1 = affine.load %arg1[%arg2, %arg3] : memref<3x2xf64>
            %2 = arith.addf %0, %1 : f64
            affine.store %2, %alloc[%arg2, %arg3] : memref<3x2xf64>
          }
        }
        return %alloc : memref<3x2xf64>
      }
    }
    """,
        "inputs": [{"value": [[1.0, 3.0], [5.0, 7.0], [9.0, 11.0]]}, {"value": [[2.0, 4.0], [6.0, 8.0], [10.0, 12.0]]}],
        "expectOutputs": [{"value": [[3.0, 7.0], [11.0, 15.0], [19.0, 23.0]]}],
        "isSim": True,
    },
    {
        "mlirContent": """
    module {
        func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"},
                              %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
            %5 = arith.mulf %arg0, %arg1 :  f32
            return %5 : f32
        }
    }
    """,
        "inputs": [{"value": [1.23]}, {"value": [4.5678]}],
        "expectOutputs": [{"value": [5.618394]}],
    },
    {
        "mlirContent": """
    module {
        func.func @euclidean_distance(%arg0: memref<4xf64>, %arg1: memref<4xf64>) -> f64 {
            %sum_init = arith.constant 0.0 : f64
            // Create affine loop with 4 iterations (0 to 3 inclusive)
            %result = affine.for %i = 0 to 4 iter_args(%acc = %sum_init) -> f64 {
                %1 = memref.load %arg0[%i] : memref<4xf64>  // array1[i]
                %2 = memref.load %arg1[%i] : memref<4xf64>  // array2[i]
                %3 = arith.subf %1, %2 : f64
                %4 = memref.load %arg0[%i] : memref<4xf64>  // array1[i]
                %5 = memref.load %arg1[%i] : memref<4xf64>  // array2[i]
                %6 = arith.subf %4, %5 : f64
                %7 = arith.mulf %3, %6 : f64
                %8 = arith.addf %acc, %7 : f64
                // Pass updated accumulator to next iteration
                affine.yield %8 : f64
            }
            // Return final accumulated euclidean distance
            return %result : f64
        }
    }
    """,
        "inputs": [{"value": [1.1, 2.2, 3.3, 4.4]}, {"value": [5.5, 6.6, 7.7, 8.8]}],
        "expectOutputs": [{"value": [77.44]}],
    },
    {
        "mlirContent": """
    module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "add"} {
      func.func @main_graph(%arg0: memref<3x2xf32> {onnx.name = "X1", onnx.type = "encrypted"},
                            %arg1: memref<3x2xf32> {onnx.name = "X2", onnx.type = "encrypted"})
                            -> (memref<3x2xf32> {onnx.name = "Y"}) attributes {llvm.emit_c_interface} {
        %alloc = memref.alloc() {alignment = 16 : i64} : memref<3x2xf32>
        affine.for %arg2 = 0 to 3 {
          affine.for %arg3 = 0 to 2 {
            %0 = affine.load %arg0[%arg2, %arg3] : memref<3x2xf32>
            %1 = affine.load %arg1[%arg2, %arg3] : memref<3x2xf32>
            %2 = arith.addf %0, %1 : f32
            affine.store %2, %alloc[%arg2, %arg3] : memref<3x2xf32>
          }
        }
        return %alloc : memref<3x2xf32>
      }
    }""",
        "inputs": [{"value": [[1.0, 3.0], [5.0, 7.0], [9.0, 11.0]]}, {"value": [[2.0, 4.0], [6.0, 8.0], [10.0, 12.0]]}],
        "expectOutputs": [{"value": [[3.0, 7.0], [11.0, 15.0], [19.0, 23.0]]}],
    },
    {
        "mlirContent": """
    module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "add"} {
      func.func @main_graph(%arg0: memref<3x2xf32> {onnx.name = "X1", onnx.type = "encrypted"},
                            %arg1: memref<3x2xf32> {onnx.name = "X2", onnx.type = "clear"})
                            -> (memref<3x2xf32> {onnx.name = "Y"}) attributes {llvm.emit_c_interface} {
        %alloc = memref.alloc() {alignment = 16 : i64} : memref<3x2xf32>
        affine.for %arg2 = 0 to 3 {
          affine.for %arg3 = 0 to 2 {
            %0 = affine.load %arg0[%arg2, %arg3] : memref<3x2xf32>
            %1 = affine.load %arg1[%arg2, %arg3] : memref<3x2xf32>
            %2 = arith.addf %0, %1 : f32
            affine.store %2, %alloc[%arg2, %arg3] : memref<3x2xf32>
          }
        }
        return %alloc : memref<3x2xf32>
      }
    }""",
        "inputs": [
            {"value": [[1.0, 3.0], [5.0, 7.0], [9.0, 11.0]]},
            {"value": [[2.0, 4.0], [6.0, 8.0], [10.0, 12.0]], "type": "clear"},
        ],
        "expectOutputs": [{"value": [[3.0, 7.0], [11.0, 15.0], [19.0, 23.0]]}],
    },
]
if CASE_INDEX >= len(TEST_CASES):
    print("ALL DONE")
    exit(1)

TEST_CASE = TEST_CASES[CASE_INDEX]

# some pre-defined values
testIsSim = True if "isSim" in TEST_CASE.keys() else False
testMlirContent = TEST_CASE["mlirContent"]
testInputs = TEST_CASE["inputs"]
testInputsLength = len(testInputs)
testOutputs = TEST_CASE["expectOutputs"]
