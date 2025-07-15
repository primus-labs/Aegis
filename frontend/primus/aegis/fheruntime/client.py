from .data_processor import FHEDataProcessor
from .keyset_manager import FHEKeysetManager
from .compiler import FHECompileResult
from primus.lib.primus_aegis import Value
import numpy as np
import tempfile
import shutil
from typing import List, Union

class FHEClient:
    """
    FHEClient class, used to generate FHE keys, encrypt and decrypt
    """
    _data_processor: FHEDataProcessor
    _keyset_manager: FHEKeysetManager
    _are_keys_loaded: bool
    _is_local_mode: bool
    _is_sim: bool

    def __init__(self, compile_result_or_file: Union[FHECompileResult, str], is_local_mode: bool = False, is_sim: bool = False):
        """
        Construct FHEClient instance

        Args
            compile_result_or_file (Union[FHECompileResult, str]):
            Accept either
                - FHECompileResult: compile result struct
                - str: either a zip format file or a json format file
            is_local_mode (bool):
                True if it works in local mode
            is_sim (bool):
                True if it works in simulate mode

        raise
            RuntimeError: if compile_result_or_file is type `str`, and it is neither a zip format file nor a json format file
        """
        if isinstance(compile_result_or_file, str):
            if compile_result_or_file.endswith('.zip'):
                compile_result_or_file = self.load(compile_result_or_file) 
            elif compile_result_or_file.endswith('.json'):
                prog_spec_file = compile_result_or_file
            else:
                raise RuntimeError('not supported keygen parameter')

        if isinstance(compile_result_or_file, FHECompileResult):
            prog_spec_file = compile_result_or_file.get_prog_spec_file_path()
        self._prog_spec_file = prog_spec_file
        self._data_processor = FHEDataProcessor(is_sim)
        self._keyset_manager = FHEKeysetManager()
        self._are_keys_loaded = False
        self._is_local_mode = is_local_mode
        self._is_sim = is_sim

    def _require_keys_loaded(self):
        """
        Check whether keys are loaded

        raise
            RuntimeError:
                If keys are not loaded
        """
        if self._is_local_mode or self._is_sim:
            return
        if not self._are_keys_loaded:
            raise RuntimeError("keys are not loaded")

    def load(self, archive_path: str) -> FHECompileResult:
        """
        Load archive file

        Args
            archive_path (str):
                The path which points to the archive

        Returns
            FHECompileResult: reeturn the compile result
            which provide the necessary information to find the needed files
        """
        tmp_dir = tempfile.mkdtemp()
        shutil.unpack_archive(archive_path, tmp_dir, 'zip')
        with open(f'{tmp_dir}/compile_result.json', 'r') as f:
            content = f.read()
        self._compile_result = FHECompileResult.from_json(content)
        self._compile_result.set_output_dir_path(tmp_dir)
        return self._compile_result


    def keygen(self):
        """
        Generate FHE keys
        """
        if self._is_sim:
            return
        self._keyset_manager.keygen(self._prog_spec_file)
        self._are_keys_loaded = True

    def save_all_keys(self, key_file_path: str):
        """
        Save all keys

        Args
            key_file_path (str):
                The location where all keys will be saved
        """
        if self._is_sim:
            return
        self._require_keys_loaded()
        self._keyset_manager.save_all_keys(key_file_path)

    def save_eva_keys(self, key_file_path: str):
        """
        Save eva keys

        Args
            key_file_path (str)
                The location where eva keys will be saved
        """
        if self._is_sim:
            return
        self._require_keys_loaded()
        self._keyset_manager.save_eva_keys(key_file_path)

    def load_all_keys(self, key_file_path: str):
        """
        Load all keys

        Args
            key_file_path (str):
                The location from which all keys will be loaded
        """
        if self._is_sim:
            return
        self._keyset_manager.load_keys(key_file_path)
        self._are_keys_loaded = True

    def encrypt(self, plain_input: Union[np.ndarray, List[np.ndarray]], serialize_output: bool = False) -> Union[Value, bytes, List[Value], List[bytes]]:
        """
        Encrypt plaintext

        Args
            plain_input (Union[np.ndarray, List[np.ndarray]):
                Accept either
                    - np.ndarray: a single piece of plaintext
                    - List[np.ndarray]: an array of pieces of plaintext
            serialize_output (bool):
                True if serialize output else False

        Returns Union[Value, bytes, List[Value], List[bytes]] return either `Value`/`List[Value]` or `bytes`/`List[bytes]` according to `serialize_output`
        """
        self._require_keys_loaded()
        if isinstance(plain_input, List):
            return [self.encrypt(i, serialize_output) for i in plain_input]

        output = self._data_processor.encrypt(plain_input)
        if serialize_output:
            output = output.to_bytes()
        return output

    def plaintext(self, plain_input: Union[np.ndarray, List[np.ndarray]], serialize_output: bool = False) -> Union[Value, bytes, List[Value], List[bytes]]:
        """
        Convert np.ndarray to Value

        Args
            plain_input (Union[np.ndarray, List[np.ndarray]):
                Accept either
                    - np.ndarray: a single piece of plaintext
                    - List[np.ndarray]: an array of pieces of plaintext
            serialize_output (bool):
                True if serialize output else False

        Returns Union[Value, bytes, List[Value], List[bytes]] return either `Value`/`List[Value]` or `bytes`/`List[bytes]` according to `serialize_output`
        """
        self._require_keys_loaded()
        if isinstance(plain_input, List):
            return [self.encrypt(i, serialize_output) for i in plain_input]

        output = self._data_processor.plaintext(plain_input)
        if serialize_output:
            output = output.to_bytes()
        return output

    def decrypt(self, ciphertext_input: Union[Value, bytes, List[Value], List[bytes]]) -> Union[np.ndarray, List[np.ndarray]]:
        """
        Decrypt ciphertext

        Args
            ciphertext_input (Union[Value, bytes, List[Value], List[bytes]]
                Accept either
                - Value
                - bytes
                - List[Value]
                - List[bytes]

        Returns
            Union[np.ndarray, List[np.ndarray]] returns decrypted plaintext
        """
        self._require_keys_loaded()
        if isinstance(ciphertext_input, List):
            return [self.decrypt(i) for i in ciphertext_input]

        if isinstance(ciphertext_input, bytes):
            ciphertext_input = Value.from_bytes(ciphertext_input)
        return self._data_processor.decrypt(ciphertext_input)
