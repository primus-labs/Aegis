from primus.aegis.fheruntime import FHEServer as Server, FHEClient as Client
import numpy as np


# define target function
def mul(X1: float, X2: float) -> float:
    return X1 * X2


# compile the target function
server = Server()
compile_result = server.compile(mul, {"X1": "encrypted", "X2": "clear"})

# key gen
client = Client(compile_result)
client.keygen()
eva_keys = "/tmp/eva_keys.bin"
client.save_eva_keys(eva_keys)

# encrypt
enc_val1 = client.encrypt(5)
pt_vale = client.plaintext(10)

# run
server.load_eva_keys(eva_keys)
enc_output = server.run([enc_val1, pt_vale])

# decrypt
pt_output = client.decrypt(enc_output)
print(pt_output)
"""
[array([50.])]
"""
