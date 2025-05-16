from .server import FHEServer
from primus_aegis.compiler import CompileOption, COMPILE_TARGET, CompileResult
from primus_aegis import Value
from typing import List

class FHEInferenceSession:
    _server: FHEServer
    _archive_path: str

    def __init__(self, onnx_file: str = None, compile_option: CompileOption = None):
        self._server = FHEServer()
        if onnx_file != None:
            mlir_file = self._server.convert_onnx_to_mlir(onnx_file)
            self._archive_path = self._server.compile(mlir_file, compile_option)

    def get_server(self) -> FHEServer:
        return self._server

    def get_archive_path(self) -> str:
        return self._archive_path

    def run(self, private_data: Value | List[Value], archive_path: str) -> Value | List[Value]:
        return self._server.run(private_data, archive_path)

    def deserialize_run_serialize(self, private_data: bytes | List[bytes], archive_path: str) -> bytes | List[bytes]:
        return self._server.deserialize_run_serialize(private_data, archive_path)
