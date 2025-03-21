
add_mlir_library(
    AegisLowerCastToEmitcStub
    LowerCastToEmitcStub.cpp
)

target_include_directories(AegisLowerCastToEmitcStub PUBLIC ${PROJECT_SOURCE_DIR}/include)