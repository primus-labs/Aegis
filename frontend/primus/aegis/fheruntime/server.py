from .compiler import Compiler
from .runtime import Runtime
from .keyset_manager import KeysetManager
from primus_aegis.compiler import CompileOption, CompileResult
from primus_aegis import Value
from typing import List

class Server:
    _compiler: Compiler
    _runtime: Runtime
    _keyset_manager: KeysetManager
    _are_keys_loaded: bool

    def __init__(self):
        self._compiler = Compiler()
        self._runtime = Runtime()
        self._keyset_manager = KeysetManager()
        self._are_keys_loaded = False

    def _require_keys_loaded(self):
        if not self._are_keys_loaded:
            raise RuntimeError("keys are not loaded")

    def load_pub_keys(self, key_file: str):
        self._keyset_manager.load_keys(key_file)
        self._are_keys_loaded = True

    def compile(self, mlir_file: str, compile_option: CompileOption) -> CompileResult:
        return self._compiler.compile(mlir_file, compile_option)

    def run(self, private_data: Value | List[Value], compile_result: CompileResult) -> Value | List[Value]:
        self._require_keys_loaded()
        return self._runtime.run(private_data, compile_result)

    def deserialize_run_serialize(self, private_data: bytes | List[bytes], compile_result: CompileResult) -> bytes | List[bytes]:
        self._require_keys_loaded()
        return self._runtime.run(private_data, compile_result)
