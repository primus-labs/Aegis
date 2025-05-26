from primus_aegis.dataprocessor import FHEDataProcessor as DataProcessor
from primus_aegis import Value
import numpy as np

class FHEDataProcessor:
    def __init__(self):
        pass

    def encrypt(self, plain_input: np.ndarray) -> Value:
        return DataProcessor.privateInput(plain_input)

    def decrypt(self, ciphertext: Value) -> Value:
        return DataProcessor.processOutput(ciphertext)
