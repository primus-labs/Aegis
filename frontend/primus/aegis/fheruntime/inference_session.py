from .server import FHEServer
from .client import FHEClient
from primus_aegis.compiler import CompileOption, COMPILE_TARGET, CompileResult
from primus_aegis import Value
from typing import List
import numpy as np

class FHEInferenceSession:
    _server: FHEServer
    _archive_path: str

    def __init__(self, onnx_file: str = None, compile_option: CompileOption = None, is_simulate: bool = False):
        self._server = FHEServer(is_simulate)
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

class LocalFHEInferenceSession(FHEInferenceSession):
    _client: FHEClient
    def __init__(self, onnx_file: str = None, compile_option: CompileOption = None):
        super().__init__(onnx_file, compile_option, True)
        self._client = FHEClient(True)
        self._client.keygen(self._server.get_output_dir() + '/prog_spec.json')

    def encrypt_run_decrypt(self, input_data: np.ndarray | List[np.ndarray]) -> np.ndarray | List[np.ndarray]:
        if isinstance(input_data, np.ndarray):
            private_data = self._client.encrypt(input_data)
        else:
            private_data = [self._client.encrypt(input_d) for input_d in input_data]
        output_data = self._server.run(private_data, self._archive_path)
        if isinstance(output_data, np.ndarray):
            decrypted_data = self._client.decrypt(output_data)
        else:
            decrypted_data = [self._client.decrypt(output_d) for output_d in output_data]
        return decrypted_data


