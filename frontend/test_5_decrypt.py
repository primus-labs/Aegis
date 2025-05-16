from primus.aegis.fheruntime import FHEClient as Client 
import numpy as np
import os

def load_data(file) -> bytes:
    with open(file, 'rb') as f:
        content = f.read()
    return content

if __name__ == '__main__':
    data_dir = os.getenv('AEGIS_DATA_DIR')
    all_keys_path = os.getenv('AEGIS_ALL_KEYS_PATH')

    client = Client()
    client.load_all_keys(all_keys_path)

    ser_output = load_data(data_dir + '/output.bin')
    output = client.deserialize_decrypt(ser_output)
    print(output)
