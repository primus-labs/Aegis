from primus.aegis.fheruntime import FHEClient as Client, FHEServer as Server, CompileOption, CompileResult, COMPILE_TARGET 
import os

if __name__ == '__main__':
    prog_spec_path = os.getenv('AEGIS_PROG_SPEC_PATH')
    all_keys_path = os.getenv('AEGIS_ALL_KEYS_PATH')
    pub_keys_path = os.getenv('AEGIS_PUB_KEYS_PATH')

    client = Client()
    client.keygen(prog_spec_path)
    client.dump_all_keys(all_keys_path)
    client.dump_pub_keys(pub_keys_path)
