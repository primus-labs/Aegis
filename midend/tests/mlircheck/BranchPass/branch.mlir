// RUN: aegiscompiler --branch --canonicalize --cse < %s | FileCheck %s


module  {
    //===----------------------------------------------------------------------===//
    // Case 1: Basic Single Branch (with else)
    //===----------------------------------------------------------------------===//
    func.func @basic(%cond: i1, %a: f32, %b: f32) -> f32 {
        // CHECK-LABEL: func @basic
        %result = scf.if %cond -> f32 {
            %t1 = arith.addf %a, %b : f32
            scf.yield %t1 : f32
        } else {
            %e1 = arith.subf %a, %b : f32
            scf.yield %e1 : f32
        }
        // CHECK: %[[THEN:.*]] = arith.addf %arg1, %arg2 : f32
        // CHECK: %[[ELSE:.*]] = arith.subf %arg1, %arg2 : f32
        // CHECK: %{{.*}} = arith.select %arg0, %[[THEN]], %[[ELSE]] : f32
        return %result : f32
    }


    //===----------------------------------------------------------------------===//
    // Case 2: Multi-operation Branch 
    //===----------------------------------------------------------------------===//
    func.func @multi_ops(%cond: i1, %a: f32, %b: f32) -> f32 {
        // CHECK-LABEL: func @multi_ops
        %result = scf.if %cond -> f32 {
            %t1 = arith.addf %a, %b : f32
            %t2 = arith.mulf %t1, %b : f32
            scf.yield %t2 : f32
        } else {
            %e1 = arith.subf %a, %b : f32
            %e2 = arith.mulf %e1, %b : f32
            scf.yield %e2 : f32
        }
        
        // CHECK: %[[T1:.*]] = arith.addf %arg1, %arg2 : f32
        // CHECK: %[[T2:.*]] = arith.mulf %[[T1]], %arg2 : f32
        // CHECK: %[[E1:.*]] = arith.subf %arg1, %arg2 : f32
        // CHECK: %[[E2:.*]] = arith.mulf %[[E1]], %arg2 : f32
        // CHECK: %{{.*}} = arith.select %arg0, %[[T2]], %[[E2]] : f32
        return %result : f32
    }

    //===----------------------------------------------------------------------===//
    // Case 3: Empty else Block
    //===----------------------------------------------------------------------===//
    func.func @empty_else(%cond: i1, %a: f32) -> f32 {
        // CHECK-LABEL: func @empty_else
        %result = scf.if %cond -> f32 {
            %t = arith.addf %a, %a : f32
            scf.yield %t : f32
        } else {
            scf.yield %a : f32
        }
        // CHECK: %[[THEN:.*]] = arith.addf %arg1, %arg1 : f32
        // CHECK: %{{.*}} = arith.select %arg0, %[[THEN]], %arg1 : f32
        return %result : f32
    }
}
