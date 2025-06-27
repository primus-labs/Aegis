from primus.aegis.fheruntime import FHEServer as Server, FHEClient as Client
import numpy as np


# define target function
def div(dividend: float, divisor: float) -> float:
    return dividend / divisor


# compile the target function
server = Server()
compile_result = server.compile(div)

# key gen
client = Client(compile_result)
client.keygen()
eva_keys = "/tmp/eva_keys.bin"
client.save_eva_keys(eva_keys)

# encrypt
# lower_bound, upper_bound = [-10, 10]
private_data_1 = 8.8
private_data_2 = -2.2
enc_val1, enc_val2 = client.encrypt([private_data_1, private_data_2])

# run
server.load_eva_keys(eva_keys)
enc_output = server.run([enc_val1, enc_val2])

# decrypt
pt_output = client.decrypt(enc_output)
print(pt_output)
"""
[array([-4.03059399])]
"""
