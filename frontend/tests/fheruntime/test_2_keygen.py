from primus.aegis.fheruntime import FHEClient as Client 
import os

if __name__ == '__main__':
    # progSpecFile path
    # file_path = os.getenv('AEGIS_PROG_SPEC_PATH')
    # OR
    # client.zip path
    file_path = os.getenv('AEGIS_CLIENT_ZIP_PATH')

    all_keys_path = os.getenv('AEGIS_ALL_KEYS_PATH')
    pub_keys_path = os.getenv('AEGIS_PUB_KEYS_PATH')

    client = Client()
    client.keygen(file_path)
    client.dump_all_keys(all_keys_path)
    client.dump_pub_keys(pub_keys_path)
