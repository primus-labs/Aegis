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
        self._compile_result = self._server.compile(onnx_file, compile_option)
        self._archive_path = self._server.save(self._compile_result, self._compile_result.outputDirPath)

    def get_server(self) -> FHEServer:
        return self._server

    def get_archive_path(self) -> str:
        return self._archive_path

    def _compute_input_output_names(self, compile_result: CompileResult) -> (List[str], List[str]):
        prog_spec_file_path = compile_result.outputDirPath + '/' + compile_result.progSpecFileName
        with open(prog_spec_file_path, 'r') as f:
            content = f.read()
        j = json.loads(content)
        function = j['funcsInfo']['functions'][0]
        inputs = function['inputs']
        outputs = function['outputs']

        input_names = [i['name'] for i in inputs]
        output_names = [o['name'] for o in outputs]
        return (input_names, output_names)

    def _check_input_output_names(self, input_names, output_names, all_input_names, all_output_names):
        if not all([i in all_input_names for i in input_names]):
            raise RuntimeError("some input names is not valid")

        if not all([i in input_names for i in all_input_names]):
            raise RuntimeError("some input is not provided")

        if output_names != None and not all([o in all_output_names for o in output_names]):
            raise RuntimeError("some output name is not valid")

    def _compute_input_data(self, input_feed, input_names: List[str]) -> List[Value] | List[bytes]:
        input_data = [input_feed[n] for n in input_names]
        return input_data

    def _compute_output_data(self, output_data, output_names: List[str], all_output_names: List[str]) -> List[bytes] | List[Value]:
        if output_names == None:
            return output_data
        else:
            return [output_data[all_output_names.index(element)] for element in output_names]

    def run(self, output_names: List[str], input_feed, archive_path: str) -> Value | List[Value] | bytes | List[bytes]:
        compile_result = self._server.load(archive_path)
        (all_input_names, all_output_names) = self._compute_input_output_names(compile_result)
        self._check_input_output_names(list(input_feed.keys()), output_names, all_input_names, all_output_names)
        private_data = self._compute_input_data(input_feed, all_input_names)
        output_data =  self._server.run(private_data, compile_result)
        return self._compute_output_data(output_data, output_names, all_output_names)

class LocalFHEInferenceSession(FHEInferenceSession):
    _client: FHEClient
    def __init__(self, path_or_bytes: bytes | str | os.PathLike = None, compile_option: CompileOption = None):
        super().__init__(path_or_bytes, compile_option, True)
        self._client = FHEClient(True)
        self._client.keygen(self._server.get_output_dir() + '/prog_spec.json')

    def encrypt_run_decrypt(self, output_names: List[str], input_feed) -> np.ndarray | List[np.ndarray]:
        (all_input_names, all_output_names) = self._compute_input_output_names(self._compile_result)
        self._check_input_output_names(list(input_feed.keys()), output_names, all_input_names, all_output_names)
        input_data = self._compute_input_data(input_feed, all_input_names);
        private_data = self._client.encrypt(input_data)
        output_data = self._server.run(private_data, self._compile_result)
        output_data = self._compute_output_data(output_data, output_names, all_output_names)
        decrypted_data = self._client.decrypt(output_data)
        return decrypted_data


