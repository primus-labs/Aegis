from primus.aegis.fheruntime import FHEClient as Client 
import numpy as np
import os
from test_utils import save_data_array

if __name__ == '__main__':
    all_keys_path = os.getenv('AEGIS_ALL_KEYS_PATH')
    data_dir = os.getenv('AEGIS_DATA_DIR')
    file_path = os.getenv('AEGIS_CLIENT_ZIP_PATH')
    is_sim = os.getenv('IS_SIM') == '1'

    client = Client(file_path, is_sim = is_sim)
    client.load_all_keys(all_keys_path)

    private_data_1 = np.array([[1, 3], [5, 7], [9, 11]])
    private_data_2 = np.array([[2, 4], [6, 8], [10, 12]])

    # private_data_1 = np.array([1, 3, 5, 7, 9, 11])
    # private_data_2 = np.array([2, 4, 6, 8, 10, 12])

    x1 = 1.1
    x2 = 2.2
    private_data_1 = np.array(x1, dtype = np.float64)
    private_data_2 = np.array(x2, dtype = np.float64)

    ser1 = client.encrypt(private_data_1, True)
    ser2 = client.plaintext(private_data_2, True)

    save_data_array([ser1, ser2], data_dir + "/private_data.bin")
