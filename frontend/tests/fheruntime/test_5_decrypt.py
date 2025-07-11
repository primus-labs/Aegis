from primus.aegis.fheruntime import FHEClient as Client 
import numpy as np
import os
from typing import List
from test_utils import load_data_array

if __name__ == '__main__':
    data_dir = os.getenv('AEGIS_DATA_DIR')
    all_keys_path = os.getenv('AEGIS_ALL_KEYS_PATH')
    file_path = os.getenv('AEGIS_CLIENT_ZIP_PATH')
    is_sim = os.getenv('IS_SIM') == '1'

    client = Client(file_path, is_sim)
    client.load_all_keys(all_keys_path)

    ser_output = load_data_array(data_dir + '/output.bin')
    output = client.decrypt(ser_output)
    print(__file__, output)
