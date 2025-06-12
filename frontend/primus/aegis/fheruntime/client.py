from .data_processor import FHEDataProcessor
from .keyset_manager import FHEKeysetManager
from .compiler import FHECompileResult
from primus_aegis import Value
import numpy as np
from typing import List

class FHEClient:
    _data_processor: FHEDataProcessor
    _keyset_manager: FHEKeysetManager
    _are_keys_loaded: bool
    _is_simulate: bool

    def __init__(self, is_simulate: bool = False):
        self._data_processor = FHEDataProcessor()
        self._keyset_manager = FHEKeysetManager()
        self._are_keys_loaded = False
        self._is_simulate = is_simulate

    def _require_keys_loaded(self):
        if self._is_simulate:
            return
        if not self._are_keys_loaded:
            raise RuntimeError("keys are not loaded")

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

    def keygen(self, compile_result_or_file: FHECompileResult | str):
        if isinstance(compile_result_or_file, str):
            if compile_result_or_file.endswith('.zip'):
                compile_result_or_file = self.load(compile_result_or_file) 
            elif compile_result_or_file.endswith('.json'):
                prog_spec_file = compile_result_or_file
            else:
                raise RuntimeError('not supported keygen parameter')

        if isinstance(compile_result_or_file, FHECompileResult):
            prog_spec_file = compile_result_or_file.get_prog_spec_file_path()
        self._keyset_manager.keygen(prog_spec_file)
        self._are_keys_loaded = True

    def dump_all_keys(self, key_file: str):
        self._require_keys_loaded()
        self._keyset_manager.dump_all_keys(key_file)

    def dump_pub_keys(self, key_file: str):
        self._require_keys_loaded()
        self._keyset_manager.dump_pub_keys(key_file)

    def load_all_keys(self, key_file: str):
        self._keyset_manager.load_keys(key_file)
        self._are_keys_loaded = True

    def encrypt(self, plain_input: np.ndarray | List[np.ndarray], serialize_output: bool = False) -> Value | bytes:
        self._require_keys_loaded()
        if isinstance(plain_input, List):
            return [self.encrypt(i, serialize_output) for i in plain_input]

        output = self._data_processor.encrypt(plain_input)
        if serialize_output:
            output = output.to_bytes()
        return output

    def decrypt(self, ciphertext_input: Value | bytes | List[Value] | List[bytes]) -> np.ndarray:
        self._require_keys_loaded()
        if isinstance(ciphertext_input, List):
            return [self.decrypt(i) for i in ciphertext_input]

        if isinstance(ciphertext_input, bytes):
            ciphertext_input = Value.from_bytes(ciphertext_input)
        return self._data_processor.decrypt(ciphertext_input)
