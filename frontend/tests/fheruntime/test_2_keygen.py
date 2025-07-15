from primus.aegis.fheruntime import FHEClient as Client 
import os
from test_config import aegis_client_zip_path as file_path
from test_config import aegis_all_keys_path as all_keys_path
from test_config import aegis_eva_keys_path as eva_keys_path
from test_config import aegis_is_sim as is_sim

if __name__ == '__main__':
    if is_sim:
        exit(0) 

    client = Client(file_path, is_sim = is_sim)
    client.keygen()
    client.save_all_keys(all_keys_path)
    client.save_eva_keys(eva_keys_path)
