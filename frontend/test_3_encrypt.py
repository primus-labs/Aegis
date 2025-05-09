from primus.aegis.fheruntime import FHEClient as Client, FHEServer as Server, CompileOption, CompileResult, COMPILE_TARGET 
import numpy as np

def save(ser: bytes, file: str):
    with open(file, 'wb') as f:
        f.write(ser)

if __name__ == '__main__':
    client = Client()
    client.load_all_keys('data/all_keys.bin')

    private_data_1 = np.array([10, 20, 30, 40])
    private_data_2 = np.array([14, 23, 33, 44])

    ser_1 = client.encrypt_serialize(private_data_1)
    ser_2 = client.encrypt_serialize(private_data_2)

    save(ser_1, "data/private_data_1.bin")
    save(ser_2, "data/private_data_2.bin")
