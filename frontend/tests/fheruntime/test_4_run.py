from primus.aegis.fheruntime import FHEServer as Server, FHEInferenceSession as InferenceSession 
from typing import List
import os
from test_utils import load_data_array, save_data_array

if __name__ == '__main__':
    data_dir = os.getenv('AEGIS_DATA_DIR')
    output_dir = os.getenv('AEGIS_OUTPUT_DIR')
    eva_keys_path = os.getenv('AEGIS_EVA_KEYS_PATH')

    private_data = load_data_array(data_dir + '/private_data.bin')
    private_data_1 = private_data[0]
    private_data_2 = private_data[1]
    archive_path = output_dir + '/server.zip'

    test_server_api = os.getenv('TEST_SERVER_API') == '1'
    print('test_server_api:', test_server_api)

    if test_server_api:
        server = Server()
        server.load_eva_keys(eva_keys_path)
        server.load(archive_path)
        output = server.run([private_data_1, private_data_2])
    else:
        inference_session = InferenceSession()
        inference_session.get_server().load_eva_keys(eva_keys_path)
        inference_session.get_server().load(archive_path)
        output = inference_session.run(['Y'], {'X1': private_data_1, 'X2': private_data_2})

    save_data_array(output, data_dir + '/output.bin')
