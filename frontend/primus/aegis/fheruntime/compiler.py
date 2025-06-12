from primus_aegis.compiler import CompileOption, CompileResult, Compiler

class FHECompileResult:
    _compile_result: CompileResult
    _binFileName: str
    _cppFileName: str
    _progSpecFileName: str
    _outputDirFileName: str

    def __init__(self, compile_result: CompileResult):
        self._compile_result = compile_result
        self._binFileName = compile_result.binFileName
        self._cppFileName = compile_result.cppFileName
        self._progSpecFileName = compile_result.progSpecFileName
        self._outputDirPath = compile_result.outputDirPath

    def get_bin_file_name(self) -> str:
        return self._binFileName

    def get_cpp_file_name(self) -> str:
        return self._cppFileName

    def get_prog_spec_file_name(self) -> str:
        return self._progSpecFileName

    def get_output_dir_path(self) -> str:
        return self._outputDirPath

    def set_output_dir_path(self, outputDir: str):
        self._compile_result.outputDirPath = outputDir
        self._outputDirPath = outputDir

    def get_bin_file_path(self) -> str:
        return self._outputDirPath + '/' + self._binFileName

    def get_cpp_file_path(self) -> str:
        return self._outputDirPath + '/' + self._cppFileName

    def get_prog_spec_file_path(self) -> str:
        return self._outputDirPath + '/' +  self._progSpecFileName

    def to_json(self) -> str:
        return self._compile_result.to_json()

    @staticmethod
    def from_json(json_content: str) -> 'FHECompileResult':
        compile_result = CompileResult.from_json(json_content)
        fhe_compile_result = FHECompileResult(compile_result)
        return fhe_compile_result

    def unwrap(self) -> CompileResult:
        return self._compile_result

class FHECompiler:
    _compiler: Compiler

    def __init__(self):
        self._compiler = Compiler()
    
    def compile(self, mlir_file: str, compile_option: CompileOption) -> FHECompileResult:
        with open(mlir_file, 'r') as f:
            mlir_content = f.read()
        
        cpp_compile_result = self._compiler.compile(mlir_content, compile_option)
        compile_result = FHECompileResult(cpp_compile_result)
        return compile_result

