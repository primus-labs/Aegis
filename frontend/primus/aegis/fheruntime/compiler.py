from primus.lib.primus_aegis.compiler import CompileOption, CompileResult, Compiler

class FHECompileResult:
    """
    FHECompileResult class, containing the path and file names
    to locate the files the compiler generates
    """
    _compile_result: CompileResult
    _simFileName: str
    _binFileName: str
    _cppFileName: str
    _progSpecFileName: str
    _outputDirFileName: str

    def __init__(self, compile_result: CompileResult):
        self._compile_result = compile_result
        self._simFileName = compile_result.simFileName
        self._binFileName = compile_result.binFileName
        self._cppFileName = compile_result.cppFileName
        self._progSpecFileName = compile_result.progSpecFileName
        self._outputDirPath = compile_result.outputDirPath

    def _get_sim_file_name(self) -> str:
        """
        return the simulate file name
        """
        return self._simFileName

    def _get_bin_file_name(self) -> str:
        """
        return the binanry file name
        """
        return self._binFileName

    def _get_cpp_file_name(self) -> str:
        """
        return the cpp file name
        """
        return self._cppFileName

    def _get_prog_spec_file_name(self) -> str:
        """
        return the prog spec file name
        """
        return self._progSpecFileName

    def get_output_dir_path(self) -> str:
        """
        return the output dir path
        """
        return self._outputDirPath

    def set_output_dir_path(self, outputDir: str):
        """
        Set output dir path

        Args
            outputDir (str)
                the output dir to be set
        """
        self._compile_result.outputDirPath = outputDir
        self._outputDirPath = outputDir

    def get_sim_file_path(self) -> str:
        """
        return the simulate file path
        """
        return self._outputDirPath + '/' + self._simFileName

    def get_bin_file_path(self) -> str:
        """
        return the binary file path
        """
        return self._outputDirPath + '/' + self._binFileName

    def get_cpp_file_path(self) -> str:
        """
        return the cpp file path
        """
        return self._outputDirPath + '/' + self._cppFileName

    def get_prog_spec_file_path(self) -> str:
        """
        return the prog spec file path
        """
        return self._outputDirPath + '/' +  self._progSpecFileName

    def to_json(self) -> str:
        """
        serialize the struct into json and return
        """
        return self._compile_result.to_json()

    @staticmethod
    def from_json(json_content: str) -> 'FHECompileResult':
        """
        Deserialize from the json and generate the struct
        Args
            json_content (str)
                the json string, which will be deserizlied
        Returns
            FHECompileResult: return the FHECompileResult deserialized from json
        """
        compile_result = CompileResult.from_json(json_content)
        fhe_compile_result = FHECompileResult(compile_result)
        return fhe_compile_result

    def unwrap(self) -> CompileResult:
        """
        return the underlying cpp object
        """
        return self._compile_result

class FHECompiler:
    """
    FHECompiler class, used to compile mlir file into FHE operations
    """
    _compiler: Compiler

    def __init__(self):
        self._compiler = Compiler()
    
    def compile(self, mlir_file: str, compile_option: CompileOption) -> FHECompileResult:
        """
        Compile mlir file into FHE operations
        Args
            mlir_file (str):
                the mlir file, which will be compiled
            compile_option (CompileOption):
                the options for compilation

        Returns
            FHECompileResult: returns the compilation result
        """
        with open(mlir_file, 'r') as f:
            mlir_content = f.read()
        
        cpp_compile_result = self._compiler.compile(mlir_content, compile_option)
        compile_result = FHECompileResult(cpp_compile_result)
        return compile_result

