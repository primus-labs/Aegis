import os

CASE_INDEX = int(os.environ.get("CASE_INDEX", 0))
# print("CASE INDEX:", CASE_INDEX)
TEST_CASES = [
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
]
TEST_CASE = TEST_CASES[CASE_INDEX]

# some pre-defined values
testMlirContent = TEST_CASE["mlirContent"]
testInputs = TEST_CASE["inputs"]
testInputsLength = len(testInputs)
testOutputs = TEST_CASE["expectOutputs"]
