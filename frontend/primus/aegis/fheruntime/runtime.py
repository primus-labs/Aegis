from primus_aegis.runtime import FHERuntime
from primus_aegis.compiler import CompileOption, CompileResult
from primus_aegis import Value
from typing import List

class Runtime:
    _fhe_runtime: FHERuntime

    def __init__(self):
        self._fhe_runtime = FHERuntime()

    def run(self, private_data: Value | List[Value], compile_result: CompileResult) -> Value | List[Value]:
        return self._fhe_runtime.run(private_data, compile_result)

    def deserialize_run_serialize(self, private_data: bytes | List[bytes], compile_result: CompileResult) -> bytes | List[bytes]:
        if isinstance(private_data, bytes):
            deser_value = Value.from_bytes(private_data)
            output = self.run(deser_value, compile_result)
            return output.to_bytes()
        else:
            deser_values = [Value.from_bytes(data) for data in private_data]
            output = self.run(deser_values, compile_result)
            ser_values = [value.to_bytes() for value in output]
            return ser_values
