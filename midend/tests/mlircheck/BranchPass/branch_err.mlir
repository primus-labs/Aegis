// RUN: aegiscompiler --branch --canonicalize --cse < %s | FileCheck -v %s


module  { 
    //===----------------------------------------------------------------------===//
    // Multi-result Error Checking
    //===----------------------------------------------------------------------===//
    func.func @multi_result_error(%cond: i1) -> (f32, f32) {    
        %r1, %r2 = scf.if %cond -> (f32, f32) {
            %t1 = arith.constant 1.0 : f32
            %t2 = arith.constant 2.0 : f32
            scf.yield %t1, %t2 : f32, f32
        } else {
            %e1 = arith.constant 3.0 : f32
            %e2 = arith.constant 4.0 : f32
            scf.yield %e1, %e2 : f32, f32
        }
        return %r1, %r2 : f32, f32
    }
}

// CHECK: error: Multi-result scf.if is not supported

