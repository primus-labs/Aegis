from .server import FHEServer
from .client import FHEClient
from .compiler import FHECompileResult
from primus.lib.primus_aegis.compiler import CompileOption, COMPILE_TARGET
from primus.lib.primus_aegis import Value
from typing import List, Union, Optional, Dict
import numpy as np
import os
import json
import tempfile

class InputOutput:
    """
    InputOutput class
    """
    name: str

    def __init__(self, name):
        self.name = name

class FHEInferenceSession:
    """
    FHEInferenceSession class, used to run inference
    """
    _server: FHEServer
    _archive_path: Optional[tuple[str, str]]
    _param_annos: Optional[Dict[str, str]]

    def __init__(self, path_or_bytes: Optional[Union[bytes, str, os.PathLike]] = None, param_annos: Optional[Dict[str,str]] = None, compile_option: CompileOption = None, is_local_mode: bool = False, is_sim: bool = False):
        """
        Construct FHEInferenceSession instance

        Args
            path_or_bytes (Optional[Union[bytes, str, os.PathLike]]):
                Accept either
                    - bytes
                    - str
                    - os.PathLike
                    - None, compilation will be skipped
            param_annos (Optional[Dict[str, str]])
                Annotation for parameter, either encrypted/clear
            compile_option (CompileOption):
                options for compilation

            is_local_mode (bool):
                Whether it works in local mode
            is_sim (bool):
                Whether it works in sim mode
        """
        self._server = FHEServer(is_local_mode, is_sim)
        self._param_annos = param_annos
        if path_or_bytes != None:
            if isinstance(path_or_bytes, bytes):
                with tempfile.TemporaryDirectory() as tmp_dir:
                    onnx_file = tmp_dir + '/' + 'tmp.onnx'
                    with open(onnx_file, 'wb') as f:
                        f.write(path_or_bytes)
                    self._do_compile(onnx_file, param_annos, compile_option)
            else:
                self._do_compile(path_or_bytes, param_annos, compile_option)

    def _do_compile(self, onnx_file: Union[str, os.PathLike], param_annos: Dict[str, str], compile_option: CompileOption):
        """
        Compile onnx file into FHE operations

        Args
            onnx_file (Union[str, os.PathLike]]):
                Accept either
                    - str
                    - os.PathLike
            param_annos (Dict[str, str])
                Annotation for parameter, either encrypted/clear

            compile_option (CompileOption):
                options for compilation
        """
        compile_result = self._server.compile(onnx_file, param_annos, compile_option)
        self._archive_path = self._server.save(compile_result, compile_result.get_output_dir_path())

    def get_server(self) -> FHEServer:
        """
        Get the underlying FHEServer
        """
        return self._server

    def get_archive_path(self) -> Optional[tuple[str, str]]:
        """
        Get the archive path
        """
        return self._archive_path

    def get_inputs(self) -> List[InputOutput]:
        """
        Get inputs
        """
        compile_result = self._server.get_compile_result()
        (input_names, output_names) = self._compute_input_output_names(compile_result)
        return [InputOutput(name) for name in input_names]

    def get_outputs(self) -> List[InputOutput]:
        """
        Get outputs
        """
        compile_result = self._server.get_compile_result()
        (input_names, output_names) = self._compute_input_output_names(compile_result)
        return [InputOutput(name) for name in output_names]

    def _compute_input_output_names(self, compile_result: FHECompileResult) -> (List[str], List[str]):
        """
        Compute input output names

        Args
            compile_result (FHECompileResult):
                The compile result
            
        Returns
            (List[str], List[str]): return input names and output names
        """
        prog_spec_file_path = compile_result.get_prog_spec_file_path()
        with open(prog_spec_file_path, 'r') as f:
            content = f.read()
        j = json.loads(content)
        function = j['funcsInfo']['functions'][0]
        inputs = function['inputs']
        outputs = function['outputs']

        input_names = [i['name'] for i in inputs]
        output_names = [o['name'] for o in outputs]
        return (input_names, output_names)

    def _check_input_output_names(self, input_names: List[str], output_names:List[str], all_input_names: List[str], all_output_names: List[str]):
        """
        Check input output names

        Args
            input_names (List[str]):
                An array of input names
            output_names (List[str]):
                An array of output names
            all_input_names (List[str]):
                All the input names
            all_output_names (List[str]):
                All the output names

        Raises
            raise RuntimeError when
                - not all `input_names` in `all_input_names`
                - not all `all_input_names` in `input_names`
                - not all `output_names` in `all_output_names`

            
        """
        if not all([i in all_input_names for i in input_names]):
            raise RuntimeError("some input names is not valid")

        if not all([i in input_names for i in all_input_names]):
            raise RuntimeError("some input is not provided")

        if output_names != None and not all([o in all_output_names for o in output_names]):
            raise RuntimeError("some output name is not valid")

    def _compute_input_data(self, input_feed: Union[Dict[str, str], Dict[str, Value]], input_names: List[str]) -> Union[List[Value], List[bytes]]:
        """
        Compute input data according input names

        Args
            input_feed (Union[Dict[str, str], Dict[str, Value]]):
                The input data
            input_names (List[str]):
                The input names

        Returns
            &nion[List[Value], List[bytes]], return the computed input data
        """
        input_data = [input_feed[n] for n in input_names]
        return input_data

    def _compute_output_data(self, output_data: Union[List[bytes], List[Value]], output_names: List[str], all_output_names: List[str]) -> Union[List[bytes], List[Value]]:
        """
        Compute output data

        Args
            output_data (List[Value]):
                An array of output data
            output_names (List[str]):
                An array of output names
            all_output_names (List[str]):
                All the output names

        Returns
            Union[List[bytes], List[Value]]: return the computed output data
        """
        if output_names == None:
            return output_data
        else:
            return [output_data[all_output_names.index(element)] for element in output_names]

    def run(self, output_names: List[str], input_feed: Union[Dict[str, str], Dict[str, Value]]) -> Union[Value, List[Value], bytes, List[bytes]]:
        """
        Execute FHE computation

        Args
            output_names (List[str]):
                An array of output names for the computation result
            input_feed (Union[Dict[str, str], Dict[str, Value]]):
                The input data

        Returns
            Union[Value, List[Value], bytes, List[bytes]]: the computation result

        """
        compile_result = self._server.get_compile_result()
        (all_input_names, all_output_names) = self._compute_input_output_names(compile_result)
        self._check_input_output_names(list(input_feed.keys()), output_names, all_input_names, all_output_names)
        private_data = self._compute_input_data(input_feed, all_input_names)
        output_data =  self._server.run(private_data)
        return self._compute_output_data(output_data, output_names, all_output_names)

class LocalFHEInferenceSession(FHEInferenceSession):
    """
    LocalFHEInferenceSession class, used to compute FHE operations locally
    """
    _client: FHEClient
    def __init__(self, path_or_bytes: Optional[Union[bytes, str, os.PathLike]] = None, param_annos: Optional[Dict[str,str]] = None, compile_option: CompileOption = None, is_sim: bool = False):
        """
        Construct LocalFHEInferenceSession instance

        Args
            path_or_bytes (Optional[Union[bytes, str, os.PathLike]])
                Accept either
                    - bytes: onnx file content
                    - str: onnx file path
                    - os.PathLike: onnx file path
            param_annos (Dict[str, str]):
                Paramter annotations, either encrypted/clear
            compile_option (CompileOption):
                Options for compilation
            is_sim (bool):
                Wheter it works in sim mode
        """
        super().__init__(path_or_bytes, param_annos, compile_option, True, is_sim)
        compile_result = self._server.get_compile_result()
        self._client = FHEClient(compile_result, True, is_sim)
        self._client.keygen()

    def run(self, output_names: List[str], input_feed: Dict[str, np.ndarray]) -> Union[np.ndarray, List[np.ndarray]]:
        """
        Encrypt plaintext, execute computation and decrypt ciphertext

        Args
            output_names (List[str])
                An array of output names
            input_feed (Dict[str, np.ndarray])
                The input data

        Returns
            Union[np.ndarray, List[np.ndarray]]: return the computation result in plaintext format
        """
        compile_result = self._server.get_compile_result()
        (all_input_names, all_output_names) = self._compute_input_output_names(compile_result)
        self._check_input_output_names(list(input_feed.keys()), output_names, all_input_names, all_output_names)
        input_data = self._compute_input_data(input_feed, all_input_names);
        if self._param_annos:
            input_status = [self._param_annos.get(item, 'encrypted') == 'encrypted' for item in all_input_names]
        else:
            input_status = [True for i in all_input_names]
        private_data = [self._client.encrypt(input_data[i]) if input_status[i] else self._client.plaintext(input_data[i]) for i in range(len(input_status))]
        output_data = self._server.run(private_data)
        output_data = self._compute_output_data(output_data, output_names, all_output_names)
        decrypted_data = self._client.decrypt(output_data)
        return decrypted_data


