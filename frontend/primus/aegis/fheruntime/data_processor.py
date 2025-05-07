from primus_aegis.dataprocessor import FHEDataProcessor
from primus_aegis import Value
import numpy as np

class DataProcessor:
    def __init__(self):
        pass

    def encrypt(self, plain_input: np.ndarray) -> Value:
        return FHEDataProcessor.privateInput(plain_input)

    def encrypt_serialize(self, plain_input: np.ndarray) -> bytes:
        return self.encrypt(plain_input).to_bytes()

    def decrypt(self, ciphertext: Value) -> Value:
        return FHEDataProcessor.processOutput(ciphertext)

    def deserialize_decrypt(self, ciphertext: bytes) -> Value:
        value = Value.from_bytes(ciphertext)
        return self.decrypt(value)
