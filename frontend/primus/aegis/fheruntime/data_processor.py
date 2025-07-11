from primus.lib.primus_aegis.dataprocessor import FHEDataProcessor as DataProcessor, SimDataProcessor
from primus.lib.primus_aegis import Value
import numpy as np

class FHEDataProcessor:
    """
    FHEDataProcessor class, used for FHE encryption and decryption
    """
    _is_sim: bool
    def __init__(self, is_sim: bool = False):
        self._is_sim = is_sim

    def encrypt(self, plain_input: np.ndarray) -> Value:
        """
        Args
            plain_input (np.ndarray)
                plaintext, which will be encrypted
        Returns
            Value: return the ciphertext
        """
        if self._is_sim:
            return SimDataProcessor.publicInput(plain_input)
        return DataProcessor.privateInput(plain_input)

    def plaintext(self, plain_input: np.ndarray) -> Value:
        """
        Args
            plain_input (np.ndarray)
                plaintext, which will be encrypted
        Returns
            Value: return the public value 
        """
        if self._is_sim:
            return SimDataProcessor.publicInput(plain_input)
        return DataProcessor.publicInput(plain_input)

    def decrypt(self, ciphertext: Value) -> Value:
        """
        Args
            ciphertext (Value)
                ciphertext, which will be decrypted
        Returns
            Value: return the plaintext
        """
        if self._is_sim:
            return SimDataProcessor.processOutput(ciphertext)
        return DataProcessor.processOutput(ciphertext)
