from primus.aegis.fheruntime import FHEClient as Client 
import numpy as np
import os
from typing import List
from test_utils import load_data_array
from test_config import aegis_runtime_dir as runtime_dir
from test_config import aegis_all_keys_path as all_keys_path
from test_config import aegis_client_zip_path as file_path
from test_config import aegis_is_sim as is_sim

if __name__ == '__main__':
    client = Client(file_path, is_sim = is_sim)
    client.load_all_keys(all_keys_path)

    ser_output = load_data_array(f'{runtime_dir}/output.bin')
    output = client.decrypt(ser_output)
    print(__file__, output)
