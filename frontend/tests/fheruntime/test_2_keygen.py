from primus.aegis.fheruntime import FHEClient as Client 
import os

if __name__ == '__main__':
    # progSpecFile path
    # file_path = os.getenv('AEGIS_PROG_SPEC_PATH')
    # OR
    # client.zip path
    file_path = os.getenv('AEGIS_CLIENT_ZIP_PATH')

    all_keys_path = os.getenv('AEGIS_ALL_KEYS_PATH')
    eva_keys_path = os.getenv('AEGIS_EVA_KEYS_PATH')
    is_sim = os.getenv('IS_SIM') == '1'
    if is_sim:
        exit(0) 

    client = Client(file_path, is_sim = is_sim)
    client.keygen()
    client.save_all_keys(all_keys_path)
    client.save_eva_keys(eva_keys_path)
