from primus_aegis.compiler import CompileOption, CompileResult, Compiler as FHECompiler

class Compiler:
    _fhe_compiler: FHECompiler

    def __init__(self):
        self._fhe_compiler = FHECompiler()
    
    def compile(self, mlir_file: str, compile_option: CompileOption) -> CompileResult:
        with open(mlir_file, 'r') as f:
            mlir_content = f.read()
        
        compile_result = self._fhe_compiler.compile(mlir_content, compile_option)
        return compile_result

