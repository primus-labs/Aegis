from primus.lib.primus_aegis.runtime import FHERuntime as Runtime
from primus.lib.primus_aegis.compiler import CompileOption
from primus.lib.primus_aegis import Value
from typing import List, Union
from .compiler import FHECompileResult

class FHERuntime:
    """
    FHERuntime class, used to execute FHE operations
    """
    _runtime: Runtime

    def __init__(self):
        self._runtime = Runtime()

    def run(self, private_data: Union[Value, List[Value]], compile_result: FHECompileResult) -> Union[Value, List[Value]]:
        """
        Args
            private_data (Union[Value, List[Value]]
                Accept either
                    - Value: a single Value
                    - List[Value]: an array of Values
            compile_result (FHECompileResult)
                provide necessary infomation for runtime operations
        Returns
            Union[Value, List[Value]]: return either Value or List[Value]
        """
        return self._runtime.run(private_data, compile_result.unwrap())
