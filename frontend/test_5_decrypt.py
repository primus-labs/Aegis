from primus.aegis.fheruntime import FHEClient as Client, FHEServer as Server, CompileOption, CompileResult, COMPILE_TARGET 
import numpy as np

def load_data(file) -> bytes:
    with open(file, 'rb') as f:
        content = f.read()
    return content

if __name__ == '__main__':
    client = Client()
    client.load_all_keys('data/all_keys.bin')

    ser_output = load_data('data/output.bin')
    output = client.deserialize_decrypt(ser_output)
    print(output)
