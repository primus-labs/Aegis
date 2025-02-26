module {
    func.func @main_graph(%arg0: memref<8xf32> {name = "input_x", type = "encrypted"}, %arg1: memref<1xf32> {name = "input_y", type = "clear"}) -> (memref<8xf32> {name = "output", type = "encrypted"}) {
        %3 = arith.mulf %arg0, %arg1 : memref<8xf32>, memref<1xf32> -> memref<8xf32>
        %4 = arith.addf %arg0, %arg1 : memref<8xf32>, memref<1xf32> -> memref<8xf32>
        %5 = arith.mulf %3, %4 : memref<8xf32>, memref<8xf32> -> memref<8xf32>
        return %5 : memref<8xf32>
    }
}