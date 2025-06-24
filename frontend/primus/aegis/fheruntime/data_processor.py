from primus.lib.primus_aegis.dataprocessor import FHEDataProcessor as DataProcessor
from primus.lib.primus_aegis import Value
import numpy as np

class FHEDataProcessor:
    """
    FHEDataProcessor class, used for FHE encryption and decryption
    """
    def __init__(self):
        pass

    def encrypt(self, plain_input: np.ndarray) -> Value:
        """
        Args
            plain_input (np.ndarray)
                plaintext, which will be encrypted
        Returns
            Value: return the ciphertext
        """
        return DataProcessor.privateInput(plain_input)

    def plaintext(self, plain_input: np.ndarray) -> Value:
        """
        Args
            plain_input (np.ndarray)
                plaintext, which will be encrypted
        Returns
            Value: return the public value 
        """
        return DataProcessor.publicInput(plain_input)

    def decrypt(self, ciphertext: Value) -> Value:
        """
        Args
            ciphertext (Value)
                ciphertext, which will be decrypted
        Returns
            Value: return the plaintext
        """
        return DataProcessor.processOutput(ciphertext)
