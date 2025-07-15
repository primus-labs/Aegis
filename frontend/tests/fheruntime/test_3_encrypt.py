from primus.aegis.fheruntime import FHEClient as Client 
import numpy as np
import os
from test_utils import save_data_array
from test_config import aegis_all_keys_path as all_keys_path
from test_config import aegis_runtime_dir as runtime_dir
from test_config import aegis_client_zip_path as file_path
from test_config import aegis_is_sim as is_sim

if __name__ == '__main__':

    client = Client(file_path, is_sim = is_sim)
    if not is_sim:
        client.load_all_keys(all_keys_path)

    private_data_1 = np.array([[1, 3], [5, 7], [9, 11]])
    private_data_2 = np.array([[2, 4], [6, 8], [10, 12]])

    # private_data_1 = np.array([1, 3, 5, 7, 9, 11])
    # private_data_2 = np.array([2, 4, 6, 8, 10, 12])

    ser1 = client.encrypt(private_data_1, True)
    ser2 = client.plaintext(private_data_2, True)

    save_data_array([ser1, ser2], f'{runtime_dir}/private_data.bin')
