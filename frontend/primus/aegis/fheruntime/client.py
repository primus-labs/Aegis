from .data_processor import FHEDataProcessor
from .keyset_manager import FHEKeysetManager
from primus_aegis import Value
import numpy as np

class FHEClient:
    _data_processor: FHEDataProcessor
    _keyset_manager: FHEKeysetManager
    _are_keys_loaded: bool

    def __init__(self):
        self._data_processor = FHEDataProcessor()
        self._keyset_manager = FHEKeysetManager()
        self._are_keys_loaded = False

    def _require_keys_loaded(self):
        if not self._are_keys_loaded:
            raise RuntimeError("keys are not loaded")

    def keygen(self, prog_spec_file: str):
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

    def encrypt(self, plain_input: np.ndarray) -> Value:
        self._require_keys_loaded()
        return self._data_processor.encrypt(plain_input)

    def encrypt_serialize(self, plain_input: np.ndarray) -> bytes:
        self._require_keys_loaded()
        return self._data_processor.encrypt_serialize(plain_input)

    def decrypt(self, ciphertext_input: Value) -> np.ndarray:
        self._require_keys_loaded()
        return self._data_processor.decrypt(ciphertext_input)

    def deserialize_decrypt(self, ciphertext_input: bytes) -> np.ndarray:
        self._require_keys_loaded()
        return self._data_processor.deserialize_decrypt(ciphertext_input)
