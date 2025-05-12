from .compiler import FHECompiler
from .runtime import FHERuntime
from .keyset_manager import FHEKeysetManager
from primus_aegis.compiler import CompileOption, CompileResult
from primus_aegis import Value
from typing import List

class FHEServer:
    _compiler: FHECompiler
    _runtime: FHERuntime
    _keyset_manager: FHEKeysetManager
    _are_keys_loaded: bool

    def __init__(self):
        self._compiler = FHECompiler()
        self._runtime = FHERuntime()
        self._keyset_manager = FHEKeysetManager()
        self._are_keys_loaded = False

    def _require_keys_loaded(self):
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

    def compile(self, mlir_file: str, compile_option: CompileOption) -> CompileResult:
        return self._compiler.compile(mlir_file, compile_option)

    def run(self, private_data: Value | List[Value], compile_result: CompileResult) -> Value | List[Value]:
        self._require_keys_loaded()
        return self._runtime.run(private_data, compile_result)

    def deserialize_run_serialize(self, private_data: bytes | List[bytes], compile_result: CompileResult) -> bytes | List[bytes]:
        self._require_keys_loaded()
        return self._runtime.deserialize_run_serialize(private_data, compile_result)
