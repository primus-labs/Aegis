from primus.aegis.fheruntime import FHEServer as Server, FHEClient as Client
import numpy as np


# define target function
def add_2d_6elements(X1: list[list[float, 2], 3], X2: list[list[float, 2], 3]) -> list[list[float, 2], 3]:
    for i in range(3):
        for j in range(2):
            X1[i][j] = X1[i][j] + X2[i][j]
    return X1


# compile the target function
server = Server()
compile_result = server.compile(add_2d_6elements)

# key gen
client = Client(compile_result)
client.keygen()
eva_keys = "/tmp/eva_keys.bin"
client.save_eva_keys(eva_keys)

# encrypt
private_data_1 = np.array([[1.0, 3.0], [5.0, 7.0], [9.0, 11.0]])
private_data_2 = np.array([[2.0, 4.0], [6.0, 8.0], [10.0, 12.0]])
enc_val1, enc_val2 = client.encrypt([private_data_1, private_data_2])

# run
server.load_eva_keys(eva_keys)
enc_output = server.run([enc_val1, enc_val2])

# decrypt
pt_output = client.decrypt(enc_output)
print(pt_output)
"""
[array([[ 3.,  7.],
       [11., 15.],
       [19., 23.]])]
"""
