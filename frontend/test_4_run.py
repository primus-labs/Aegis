from primus.aegis.fheruntime import FHEClient as Client, FHEServer as Server, FHEInferenceSession as InferenceSession, CompileOption, CompileResult, COMPILE_TARGET 
from typing import List

def load_data(file) -> bytes:
    with open(file, 'rb') as f:
        content = f.read()
    return content

def save_data(data, file):
    with open(file, 'wb') as f:
        f.write(data)

def load_compile_result(file) -> CompileResult:
    import json
    with open(file, 'r') as f:
        content = f.read()

    j = json.loads(content)
    compile_result = CompileResult()
    compile_result.outputDirPath = j['outputDirPath']
    compile_result.cppFileName = j['cppFileName']
    compile_result.binFileName = j['binFileName']
    compile_result.progSpecFileName = j['progSpecFileName']

    return compile_result

if __name__ == '__main__':
    private_data_1 = load_data('data/private_data_1.bin')
    private_data_2 = load_data('data/private_data_2.bin')
    compile_result = load_compile_result('data/compile_result.json')

    if False:
        server = Server()
        server.load_pub_keys('data/pub_keys.bin')
        output = server.deserialize_run_serialize([private_data_1, private_data_2], compile_result)
    else:
        inference_session = InferenceSession()
        inference_session.get_server().load_pub_keys('data/pub_keys.bin')
        output = inference_session.deserialize_run_serialize([private_data_1, private_data_2], compile_result)

    save_data(output[0], 'data/output.bin')
