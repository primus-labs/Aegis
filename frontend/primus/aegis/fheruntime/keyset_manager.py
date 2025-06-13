from primus_aegis.fhe import KeyInfo, Keyset
import json

class FHEKeysetManager:
    _keyset: Keyset

    def __init__(self):
        self._keyset = Keyset.getInstance()

    def keygen(self, prog_spec_file_path: str):
        self._keyset.generate(prog_spec_file = prog_spec_file_path)

    def save_keys(self, key_file_path: str, contain_sk: bool):
        keys = self._keyset.to_bytes(contain_sk = contain_sk)
        with open(key_file_path, 'wb') as f:
            f.write(keys)

    def save_all_keys(self, key_file_path: str):
        self.save_keys(key_file_path, True)

    def save_eva_keys(self, key_file_path: str):
        self.save_keys(key_file_path, False)

    def load_keys(self, key_file_path: str):
        with open(key_file_path, 'rb') as f:
            content = f.read()
        self._keyset.from_bytes(content)

