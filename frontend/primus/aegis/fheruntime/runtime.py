from primus_aegis.runtime import FHERuntime as Runtime
from primus_aegis.compiler import CompileOption
from primus_aegis import Value
from typing import List
from .compiler import FHECompileResult

class FHERuntime:
    _runtime: Runtime

    def __init__(self):
        self._runtime = Runtime()

    def run(self, private_data: Value | List[Value], compile_result: FHECompileResult) -> Value | List[Value]:
        return self._runtime.run(private_data, compile_result.unwrap())
