from primus.aegis.fheruntime import FHEServer as Server, FHEClient as Client, FHEInferenceSession as InferenceSession
import numpy as np
import os

py_dir = os.path.dirname(os.path.abspath(__file__))
onnx_file = os.path.join(py_dir, "add.onnx")

# ---------------
# developer role
# ---------------
session = InferenceSession(onnx_file)
(client_archive_path, server_archive_path) = session.get_archive_path()
print(client_archive_path, server_archive_path)
# send server.zip to server
# send client.zip to client

# ---------------
# client role
# ---------------
# key gen
client = Client(client_archive_path)
client.keygen()

# save eva keys
eva_keys = "/tmp/eva_keys.bin"
client.save_eva_keys(eva_keys)

# encrypt data
data_1 = np.array([[1, 3], [5, 7], [9, 11]], dtype=np.float32)
data_2 = np.array([[2, 4], [6, 8], [10, 12]], dtype=np.float32)
enc_val1, enc_val2 = client.encrypt([data_1, data_2])

# send encrypt data to server
# send enc_val1, enc_val2

# ---------------
# server role
# ---------------
server = Server()
res = server.load(server_archive_path)

# load eva keys
server.load_eva_keys(eva_keys)

# run
enc_output = server.run([enc_val1, enc_val2])

# send encrypt output data to client
# send enc_output

# ---------------
# client role
# ---------------
# decrypt
pt_output = client.decrypt(enc_output)
print(pt_output)
"""
[array([[ 3.,  7.],
       [11., 15.],
       [19., 23.]])]
"""
