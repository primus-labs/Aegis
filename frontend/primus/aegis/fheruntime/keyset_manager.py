from primus_aegis.fhe import KeyInfo, Keyset
import json

class KeysetManager:
    _keyset: Keyset

    def __init__(self):
        self._keyset = Keyset.getInstance()

    def keygen(self, prog_spec_file: str):
        self._keyset.generate(prog_spec_file = prog_spec_file)

    def dump_keys(self, key_file: str, contain_sk: bool):
        keys = self._keyset.to_bytes(contain_sk = contain_sk)
        with open(key_file, 'wb') as f:
            f.write(keys)

    def dump_all_keys(self, key_file: str):
        self.dump_keys(key_file, True)

    def dump_pub_keys(self, key_file: str):
        self.dump_keys(key_file, False)

    def load_keys(self, key_file: str):
        with open(key_file, 'rb') as f:
            content = f.read()
        self._keyset.from_bytes(content)

