from .compiler import FHECompiler, FHECompileResult
from .runtime import FHERuntime
from .keyset_manager import FHEKeysetManager
from .py2mlir_converter import Py2MLIRConverter
from primus_aegis.compiler import CompileOption, COMPILE_TARGET
from primus_aegis import Value
from typing import Callable, List
import tempfile
import shutil
import os

class FHEServer:
    _compile_result: FHECompileResult
    _compiler: FHECompiler
    _runtime: FHERuntime
    _keyset_manager: FHEKeysetManager
    _are_keys_loaded: bool
    _output_dir: str
    _is_simulate: bool

    def __init__(self, is_simulate = False):
        self._compiler = FHECompiler()
        self._runtime = FHERuntime()
        self._keyset_manager = FHEKeysetManager()
        self._are_keys_loaded = False
        self._is_simulate = is_simulate
        self._output_dir = './output'
        self._compile_result = None

    def get_output_dir(self) -> str:
        return self._output_dir
    
    def get_compile_result(self) -> FHECompileResult:
        return self._compile_result

    def _require_keys_loaded(self):
        if self._is_simulate:
            return
        if not self._are_keys_loaded:
            raise RuntimeError("keys are not loaded")

    def load_eva_keys(self, key_file_path: str):
        self._keyset_manager.load_keys(key_file_path)
        self._are_keys_loaded = True

    def _convert_onnx_to_mlir(self, onnx_file: str) -> str:
        import os
        import subprocess
        cmd = ['onnx-mlir', '--EmitMLIR', onnx_file]
        result = subprocess.run(cmd, capture_output=True, text=True)
        mlir_file = onnx_file + '.mlir'
        if len(result.stderr) > 0:
            raise RuntimeError('convert onnx to mlir error: ' + result.stderr)
        cmd = ['sed','-i', '/krnl/d', mlir_file]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if len(result.stderr) > 0:
            raise RuntimeError(result.stderr)
        return mlir_file

    def _convert_py_to_mlir(self, output_dir: str, function: Callable) -> str:
        converter = Py2MLIRConverter(output_dir)
        return converter.convert(function)

    def _do_save(self, compile_result: FHECompileResult, output_dir: str, is_server: bool) -> str:
        file_name = 'server' if is_server else 'client'
        with tempfile.TemporaryDirectory() as tmp_dir:
            progSpecFileName = compile_result.get_prog_spec_file_name()
            if len(progSpecFileName) > 0:
                shutil.copyfile(compile_result.get_prog_spec_file_path(), tmp_dir + '/' + progSpecFileName)

            if is_server:
                binFileName = compile_result.get_bin_file_name()
                if len(binFileName) > 0:
                    shutil.copyfile(compile_result.get_bin_file_path(), tmp_dir + '/' +  binFileName)

                cppFileName = compile_result.get_cpp_file_name()
                if len(cppFileName) > 0:
                    shutil.copyfile(compile_result.get_cpp_file_path(), tmp_dir + '/' +  cppFileName)

            with open(tmp_dir + '/' + 'compile_result.json', 'w') as f:
                f.write(compile_result.to_json())

            shutil.make_archive(output_dir + '/' + file_name, 'zip', tmp_dir)
        return output_dir + '/' + file_name + '.zip'
    
    def save(self, compile_result: FHECompileResult, output_dir: str) -> (str, str):
        client_archive_path = self._do_save(compile_result, output_dir, False)
        server_archive_path = self._do_save(compile_result, output_dir, True)
        return (client_archive_path, server_archive_path)

    def load(self, archive_path: str) -> FHECompileResult:
        tmp_dir = tempfile.mkdtemp()
        print('unpack dir', tmp_dir)
        print(archive_path)
        shutil.unpack_archive(archive_path, tmp_dir, 'zip')
        with open(tmp_dir + '/' + 'compile_result.json', 'r') as f:
            content = f.read()
        self._compile_result = FHECompileResult.from_json(content)
        self._compile_result.set_output_dir_path(tmp_dir)
        print(self._compile_result.to_json())
        return self._compile_result

    def compile(self, onnx_file_or_py_function: str | Callable, compile_option: CompileOption = None) -> FHECompileResult:
        if compile_option == None:
            compile_option = CompileOption()
            compile_option.compileTarget = COMPILE_TARGET.LIBRARY
            compile_option.outputDir = os.getenv('AEGIS_OUTPUT_DIR', "./output")
        self._output_dir = compile_option.outputDir
        if isinstance(onnx_file_or_py_function, str):
            mlir_file = self._convert_onnx_to_mlir(onnx_file_or_py_function)
        elif isinstance(onnx_file_or_py_function, Callable):
            mlir_file = self._convert_py_to_mlir(self._output_dir, onnx_file_or_py_function)
        else:
            raise RuntimeError("onnx_file and py_function are None")

        self._compile_result = self._compiler.compile(mlir_file, compile_option)
        return self._compile_result

    def run(self, private_data: bytes | List[bytes] | Value | List[Value]) -> bytes | List[bytes] | Value | List[Value]:
        self._require_keys_loaded()
        if self._compile_result == None:
            raise RuntimeError("server.zip is not provided")

        is_input_serialized = False
        if isinstance(private_data, bytes):
            private_data = Value.from_bytes(private_data)
            is_input_serialized = True
        elif isinstance(private_data, List) and isinstance(private_data[0], bytes):
            private_data = [Value.from_bytes(data) for data in private_data]
            is_input_serialized = True

        output = self._runtime.run(private_data, self._compile_result)

        if not is_input_serialized:
            return output

        if isinstance(output, Value):
            ser_value = output.to_bytes()
        else:
            ser_value = [o.to_bytes() for o in output]

        return ser_value

