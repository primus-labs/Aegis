from primus_aegis.compiler import CompileOption, CompileResult, Compiler

class FHECompiler:
    _compiler: Compiler

    def __init__(self):
        self._compiler = Compiler()
    
    def compile(self, mlir_file: str, compile_option: CompileOption) -> CompileResult:
        with open(mlir_file, 'r') as f:
            mlir_content = f.read()
        
        compile_result = self._compiler.compile(mlir_content, compile_option)
        print('begin compile:', compile_option.to_json(), compile_result.to_json())
        return compile_result

