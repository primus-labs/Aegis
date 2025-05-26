from primus_aegis.runtime import FHERuntime as Runtime
from primus_aegis.compiler import CompileOption, CompileResult
from primus_aegis import Value
from typing import List

class FHERuntime:
    _runtime: Runtime

    def __init__(self):
        self._runtime = Runtime()

    def run(self, private_data: Value | List[Value], compile_result: CompileResult) -> Value | List[Value]:
        return self._runtime.run(private_data, compile_result)
