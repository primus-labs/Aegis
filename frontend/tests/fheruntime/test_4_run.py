from primus.aegis.fheruntime import FHEServer as Server, FHEInferenceSession as InferenceSession 
from typing import List
import os
from test_utils import load_data_array, save_data_array

if __name__ == '__main__':
    data_dir = os.getenv('AEGIS_DATA_DIR')
    output_dir = os.getenv('AEGIS_OUTPUT_DIR')
    pub_keys_path = os.getenv('AEGIS_PUB_KEYS_PATH')

    private_data = load_data_array(data_dir + '/private_data.bin')
    private_data_1 = private_data[0]
    private_data_2 = private_data[1]
    archive_path = output_dir + '/server.zip'

    test_server_api = os.getenv('TEST_SERVER_API') == '1'
    print('test_server_api:', test_server_api)

    if test_server_api:
        server = Server()
        server.load_pub_keys(pub_keys_path)
        compile_result = server.load(archive_path)
        output = server.run([private_data_1, private_data_2], compile_result)
    else:
        inference_session = InferenceSession()
        inference_session.get_server().load_pub_keys(pub_keys_path)
        output = inference_session.run(['Y'], {'X1': private_data_1, 'X2': private_data_2}, archive_path)

    save_data_array(output, data_dir + '/output.bin')
