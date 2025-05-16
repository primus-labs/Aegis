from .compiler import FHECompiler
from .runtime import FHERuntime
from .keyset_manager import FHEKeysetManager
from primus_aegis.compiler import CompileOption, CompileResult, COMPILE_TARGET
from primus_aegis import Value
from typing import List
import tempfile
import shutil

class FHEServer:
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

    def get_output_dir(self) -> str:
        return self._output_dir

    def _require_keys_loaded(self):
        if self._is_simulate:
            return
        if not self._are_keys_loaded:
            raise RuntimeError("keys are not loaded")

    def load_pub_keys(self, key_file: str):
        self._keyset_manager.load_keys(key_file)
        self._are_keys_loaded = True

    def convert_onnx_to_mlir(self, onnx_file: str) -> str:
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

    def make_archive(self, compile_result: CompileResult) -> str:
        with tempfile.TemporaryDirectory() as tmp_dir:
            if len(compile_result.progSpecFileName) > 0:
                shutil.copyfile(compile_result.outputDirPath + '/' + compile_result.progSpecFileName, tmp_dir + '/' +  compile_result.progSpecFileName)
            if len(compile_result.binFileName) > 0:
                shutil.copyfile(compile_result.outputDirPath + '/' + compile_result.binFileName, tmp_dir + '/' +  compile_result.binFileName)
            if len(compile_result.cppFileName) > 0:
                shutil.copyfile(compile_result.outputDirPath + '/' + compile_result.cppFileName, tmp_dir + '/' +  compile_result.cppFileName)

            with open(tmp_dir + '/' + 'compile_result.json', 'w') as f:
                f.write(compile_result.to_json())

            shutil.make_archive(compile_result.outputDirPath + '/' + 'server', 'zip', tmp_dir)
        return compile_result.outputDirPath + '/' + 'server.zip'
    
    def unpack_archive(self, archive_path: str) -> CompileResult:
        tmp_dir = tempfile.mkdtemp()
        print('unpack dir', tmp_dir)
        shutil.unpack_archive(archive_path, tmp_dir, 'zip')
        with open(tmp_dir + '/' + 'compile_result.json', 'r') as f:
            content = f.read()
        compile_result = CompileResult.from_json(content)
        compile_result.outputDirPath = tmp_dir
        print(compile_result.to_json())
        return compile_result

    def compile(self, mlir_file: str, compile_option: CompileOption = None) -> str:
        if compile_option == None:
            compile_option = CompileOption()
            compile_option.compileTarget = COMPILE_TARGET.LIBRARY
            compile_option.outputDir = "./output"
        self._output_dir = compile_option.outputDir

        compile_result = self._compiler.compile(mlir_file, compile_option)
        archive_path = self.make_archive(compile_result)
        return archive_path

    def run(self, private_data: Value | List[Value], archive_path: str) -> Value | List[Value]:
        self._require_keys_loaded()
        compile_result = self.unpack_archive(archive_path)
        return self._runtime.run(private_data, compile_result)

    def deserialize_run_serialize(self, private_data: bytes | List[bytes], archive_path: str) -> bytes | List[bytes]:
        self._require_keys_loaded()
        compile_result = self.unpack_archive(archive_path)
        return self._runtime.deserialize_run_serialize(private_data, compile_result)
