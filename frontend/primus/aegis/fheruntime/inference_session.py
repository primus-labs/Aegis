from .server import FHEServer
from .client import FHEClient
from primus_aegis.compiler import CompileOption, COMPILE_TARGET, CompileResult
from primus_aegis import Value
from typing import List
import numpy as np
import os
import json
import tempfile

class FHEInferenceSession:
    _server: FHEServer
    _compile_result: CompileResult
    _archive_path: str

    def __init__(self, path_or_bytes: bytes | str | os.PathLike = None, compile_option: CompileOption = None, is_simulate: bool = False):
        self._server = FHEServer(is_simulate)
        if path_or_bytes != None:
            if isinstance(path_or_bytes, bytes):
                with tempfile.TemporaryDirectory() as tmp_dir:
                    onnx_file = tmp_dir + '/' + 'tmp.onnx'
                    with open(onnx_file, 'wb') as f:
                        f.write(path_or_bytes)
                    self._do_compile(onnx_file, compile_option)
            else:
                self._do_compile(path_or_bytes, compile_option)

    def _do_compile(self, onnx_file: str | os.PathLike, compile_option: CompileOption):
        mlir_file = self._server.convert_onnx_to_mlir(onnx_file)
        self._compile_result = self._server.compile(mlir_file, compile_option)
        self._archive_path = self._server.save(self._compile_result, self._compile_result.outputDirPath)

    def get_server(self) -> FHEServer:
        return self._server

    def get_archive_path(self) -> str:
        return self._archive_path

    def compute_input_data(self, input_feed, compile_result: CompileResult) -> List[Value] | List[bytes]:
        prog_spec_file_path = compile_result.outputDirPath + '/' + compile_result.progSpecFileName
        with open(prog_spec_file_path, 'r') as f:
            content = f.read()
        j = json.loads(content)
        inputs = j['funcsInfo']['functions'][0]['inputs']

        input_names = [i['name'] for i in inputs]
        input_data = [input_feed[n] for n in input_names]
        return input_data

    def run(self, output_names: List[str], input_feed, archive_path: str) -> Value | List[Value]:
        compile_result = self._server.load(archive_path)
        private_data = self.compute_input_data(input_feed, compile_result)
        return self._server.run(private_data, compile_result)

    def deserialize_run_serialize(self, output_names: List[str], input_feed, archive_path: str) -> bytes | List[bytes]:
        compile_result = self._server.load(archive_path)
        private_data = self.compute_input_data(input_feed, compile_result)
        return self._server.deserialize_run_serialize(private_data, compile_result)

class LocalFHEInferenceSession(FHEInferenceSession):
    _client: FHEClient
    def __init__(self, path_or_bytes: bytes | str | os.PathLike = None, compile_option: CompileOption = None):
        super().__init__(path_or_bytes, compile_option, True)
        self._client = FHEClient(True)
        self._client.keygen(self._server.get_output_dir() + '/prog_spec.json')

    def encrypt_run_decrypt(self, output_names: List[str], input_feed) -> np.ndarray | List[np.ndarray]:
        input_data = self.compute_input_data(input_feed, self._compile_result);
        if isinstance(input_data, np.ndarray):
            private_data = self._client.encrypt(input_data)
        else:
            private_data = [self._client.encrypt(input_d) for input_d in input_data]
        output_data = self._server.run(private_data, self._compile_result)
        if isinstance(output_data, np.ndarray):
            decrypted_data = self._client.decrypt(output_data)
        else:
            decrypted_data = [self._client.decrypt(output_d) for output_d in output_data]
        return decrypted_data


