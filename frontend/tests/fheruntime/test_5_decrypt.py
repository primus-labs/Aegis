from primus.aegis.fheruntime import FHEClient as Client 
import numpy as np
import os
from typing import List

def load_data(file) -> List[bytes]:
    with open(file + '.num', 'r') as f:
        size = int(f.read())

    lst = []
    for i in range(size):
        with open(file + '.' + str(i), 'rb') as f:
            content = f.read()
            lst.append(content)
    return lst

if __name__ == '__main__':
    data_dir = os.getenv('AEGIS_DATA_DIR')
    all_keys_path = os.getenv('AEGIS_ALL_KEYS_PATH')

    client = Client()
    client.load_all_keys(all_keys_path)

    ser_output = load_data(data_dir + '/output.bin')
    output = [client.deserialize_decrypt(o) for o in ser_output]
    print(output)
