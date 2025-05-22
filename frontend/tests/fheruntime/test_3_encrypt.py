from primus.aegis.fheruntime import FHEClient as Client 
import numpy as np
import os

def save(ser: bytes, file: str):
    with open(file, 'wb') as f:
        f.write(ser)

if __name__ == '__main__':
    all_keys_path = os.getenv('AEGIS_ALL_KEYS_PATH')
    data_dir = os.getenv('AEGIS_DATA_DIR')

    client = Client()
    client.load_all_keys(all_keys_path)

    private_data_1 = np.array([[1, 3], [5, 7], [9, 11]])
    private_data_2 = np.array([[2, 4], [6, 8], [10, 12]])

    private_data_1 = np.array([1, 3, 5, 7, 9, 11])
    private_data_2 = np.array([2, 4, 6, 8, 10, 12])

    ser_1 = client.encrypt_serialize(private_data_1)
    ser_2 = client.encrypt_serialize(private_data_2)

    save(ser_1, data_dir + "/private_data_1.bin")
    save(ser_2, data_dir + "/private_data_2.bin")
