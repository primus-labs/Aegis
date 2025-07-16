import os

def get_dirname(file_name):
    return file_name[:file_name.rfind('/')]

aegis_output_dir = '/tmp/aegis_output'
aegis_prog_spec_path = f'{aegis_output_dir}/prog_spec.json'
aegis_client_zip_path = f'{aegis_output_dir}/client.zip'

_cur_dirname = get_dirname(__file__)
aegis_data_dir = f'{_cur_dirname}/data'
aegis_onnx_file_path = f'{aegis_data_dir}/add.onnx'

aegis_runtime_dir = f'/tmp/aegis_runtime'
aegis_all_keys_path = f'{aegis_runtime_dir}/all_keys.bin'
aegis_eva_keys_path = f'{aegis_runtime_dir}/eva_keys.bin'

aegis_test_server_api = os.getenv('AEGIS_TEST_SERVER_API', 'ON') == 'ON'
aegis_is_sim = os.getenv('AEGIS_IS_SIM', 'OFF') == 'ON'
