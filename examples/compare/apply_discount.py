from primus.aegis.fheruntime import FHEServer as Server, FHEClient as Client 
import numpy as np

# define target function
def apply_discount(member_level: float, price: float) -> float:
    result = 0.0
    if member_level >= 2.0:
        result = price * 0.75
    else:
        result = price * 1.0
    return result

# compile the target function
server = Server()
compile_result = server.compile(apply_discount)

# key gen
client = Client(compile_result)
client.keygen()
eva_keys = "/tmp/eva_keys.bin"
client.save_eva_keys(eva_keys)

# encrypte
private_data_1 = 2.1
private_data_2 = 120
enc_val1, enc_val2 = client.encrypt([private_data_1, private_data_2])

# run
server.load_eva_keys(eva_keys)
enc_output = server.run([enc_val1, enc_val2])

# decrypte
pt_output = client.decrypt(enc_output)
print(pt_output)