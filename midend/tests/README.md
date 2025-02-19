## lit (LLVM integrated tester) Tests

The MLIR tests use lit (LLVM integrated tester). lit allows specifying tests in-line in *.mlir files (using a RUN ... line). This is used to run FileCheck tests that check MLIR lowerings/conversions, but can also be used to simply verify that an *.mlir file parses correctly. This setup seems to have been originally designed for regression tests in LLVM, so see Regression Test Structure for information on how this works.

There is a "check-heco" target in the CMake file which can be used to run these tests. This will search for *.mlir files and try to run then. Note that the test happen during the build phase of the CMake process, there is no way of running these targets.