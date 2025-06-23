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

# encrypte
private_data_1 = np.array([[1., 3.], 
                           [5., 7.], 
                           [9., 11.]])
private_data_2 = np.array([[2., 4.], 
                           [6., 8.], 
                           [10., 12.]])
enc_val1, enc_val2 = client.encrypt([private_data_1, private_data_2])

# run
server.load_eva_keys(eva_keys)
enc_output = server.run([enc_val1, enc_val2])

# decrypte
pt_output = client.decrypt(enc_output)
print(pt_output)