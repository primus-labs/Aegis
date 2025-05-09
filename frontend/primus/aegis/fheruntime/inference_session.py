from .server import FHEServer
from primus_aegis.compiler import CompileOption, COMPILE_TARGET, CompileResult
from primus_aegis import Value
from typing import List

class FHEInferenceSession:
    _server: FHEServer
    _compile_result: CompileResult

    def __init__(self, onnx_file: str = None):
        self._server = FHEServer()
        if onnx_file != None:
            mlir_file = self._server.convert_onnx_to_mlir(onnx_file)
            compile_option = CompileOption()
            compile_option.compileTarget = COMPILE_TARGET.LIBRARY
            compile_option.outputDir = './'

            self._compile_result = self._server.compile(mlir_file, compile_option)

    def get_server(self) -> FHEServer:
        return self._server

    def get_compile_result(self) -> CompileResult:
        return self._compile_result

    def run(self, private_data: Value | List[Value], compile_result: CompileResult) -> Value | List[Value]:
        return self._server.run(private_data, compile_result)

    def deserialize_run_serialize(self, private_data: bytes | List[bytes], compile_result: CompileResult) -> bytes | List[bytes]:
        return self._server.deserialize_run_serialize(private_data, compile_result)
