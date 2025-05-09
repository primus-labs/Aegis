from primus.aegis.fheruntime import FHEClient as Client, FHEServer as Server, CompileOption, CompileResult, COMPILE_TARGET 

if __name__ == '__main__':
    client = Client()
    client.keygen('prog_spec.json')
    client.dump_all_keys('data/all_keys.bin')
    client.dump_pub_keys('data/pub_keys.bin')
