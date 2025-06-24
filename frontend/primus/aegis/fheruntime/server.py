from .compiler import FHECompiler, FHECompileResult
from .runtime import FHERuntime
from .keyset_manager import FHEKeysetManager
from .py2mlir_converter import Py2MLIRConverter
from primus.lib.primus_aegis.compiler import CompileOption, COMPILE_TARGET
from primus.lib.primus_aegis import Value
from typing import Callable, List, Union, Dict, Optional
import tempfile
import shutil
import os
import re
import inspect
from .mlir_tool import apply_mlir

class FHEServer:
    """
    FHEServer class, used to compile onnx file or python code to FHE operations
    """
    _compile_result: FHECompileResult
    _compiler: FHECompiler
    _runtime: FHERuntime
    _keyset_manager: FHEKeysetManager
    _are_keys_loaded: bool
    _output_dir: str
    _is_simulate: bool

    def __init__(self, is_simulate: bool = False):
        """
        Construct FHEServer
        Args
            is_simulate (bool):
                Whether it works in simulate mode
        """
        self._compiler = FHECompiler()
        self._runtime = FHERuntime()
        self._keyset_manager = FHEKeysetManager()
        self._are_keys_loaded = False
        self._is_simulate = is_simulate
        self._output_dir = './output'
        self._compile_result = None

    def get_output_dir(self) -> str:
        """
        Return output dir
        """
        return self._output_dir
    
    def get_compile_result(self) -> FHECompileResult:
        """
        Return compile result
        """
        return self._compile_result

    def _require_keys_loaded(self):
        """
        Check whether keys are loaded
        """
        if self._is_simulate:
            return
        if not self._are_keys_loaded:
            raise RuntimeError("keys are not loaded")

    def load_eva_keys(self, key_file_path: str):
        """
        Load eva keys

        Args
            key_file_path (str):
                The location from which eva keys are loaded
        """
        self._keyset_manager.load_keys(key_file_path)
        self._are_keys_loaded = True

    def _convert_onnx_to_mlir(self, onnx_file: str) -> str:
        """
        Convert onnx file to mlir file

        Args
            onnx_file (str):
                The onnx file which willl be converted to mlir file

        Returns
            str: return the mlir file path
        """
        import os
        import subprocess
        cmd = ['onnx-mlir', '--EmitMLIR', onnx_file]
        result = subprocess.run(cmd, capture_output=True, text=True)
        mlir_file = onnx_file + '.mlir'
        if len(result.stderr) > 0:
            raise RuntimeError('convert onnx to mlir error: ' + result.stderr)
        
        # apply mlir content
        with open(mlir_file, 'r', encoding='utf-8') as f:
          input_mlir = f.read()
          output_mlir = apply_mlir(input_mlir)
        with open(mlir_file, 'w', encoding='utf-8') as f:
          f.write(output_mlir)
        return mlir_file

    def _modify_mlir_file(self, param_annos: Dict[str, str], param_names: List[str], mlir_file: str):
        """
        Modify mlir file, add param annotations

        Args
            param_annos (Dict[str, str]):
                The param annotations, either encrypted or clear
            param_names (List[str])
                The param name
            mlir_file (str)
                The mlir file which willl be modified

        Returns
            str: return the mlir file path
        """
        with open(mlir_file, 'r') as f:
            content = f.read()
        match_fn = re.search('func.func.*{', content)
        fn_str = match_fn.group()
    
        match_args = re.search('\((.*?)\)', fn_str)
        args_str = match_args.group(1)
    
        match_arg = re.findall('([%\w]+)\s*:\s*([<>\w]+)\s*{([^}]+)}', args_str)
        has_onnx_name = True
        if len(match_arg) == 0:
            match_arg = re.findall('([%\w]+)\s*:\s*([<>\w]+)', args_str)
            has_onnx_name = False
        arg_lst = []
        for i in range(len(match_arg)):
            if has_onnx_name:
                name = re.search('onnx.name\s*=\s*\"(\w+)\"', match_arg[i][2]).group(1)
                name_and_type = match_arg[i][2] + ', onnx.type = "' + param_annos[name] + '"'
            else:
                name_and_type = 'onnx.name = "' + param_names[i] + '", onnx.type = "' + param_annos[param_names[i]] + '"'
            arg_lst.append((match_arg[i][0], match_arg[i][1], name_and_type))
    
        args_str2 = ''
        for arg in arg_lst:
            if len(args_str2) > 0:
                args_str2 += ', '
            args_str2 += arg[0] + ': ' + arg[1] + ' {' + arg[2] + '}'
    
        sub_fn = re.sub('\((.*?)\)\s*->', '(' + args_str2 + ') ->', fn_str)
        sub_content = re.sub('func.func.*{', sub_fn, content)

        with open(mlir_file, 'w') as f:
            f.write(sub_content)

    def _convert_py_to_mlir(self, output_dir: str, function: Callable) -> str:
        """
        Convert python code to mlir code

        Args
            output_dir (str):
                The location where the generated mlir file will be put
            function (Callable):
                The python function
        
        Returns
            str: return the mlir file path
        """
        converter = Py2MLIRConverter(output_dir)
        return converter.convert(function)

    def _do_save(self, compile_result: FHECompileResult, output_dir: str, is_server: bool) -> str:
        """
        Save files generated during compilation into the zip format file

        Args
            compile_result (FHECompileResult)
                The collection of files generated during compilation
            output_dir (str):
                The location where the generated zip format file will be put
            is_server (bool):
                True if save for the server else False

        Returns
            str: return the zip format file path
        """
        file_name = 'server' if is_server else 'client'
        with tempfile.TemporaryDirectory() as tmp_dir:
            progSpecFileName = compile_result._get_prog_spec_file_name()
            if len(progSpecFileName) > 0:
                shutil.copyfile(compile_result.get_prog_spec_file_path(), tmp_dir + '/' + progSpecFileName)

            if is_server:
                binFileName = compile_result._get_bin_file_name()
                if len(binFileName) > 0:
                    shutil.copyfile(compile_result.get_bin_file_path(), tmp_dir + '/' +  binFileName)

                cppFileName = compile_result._get_cpp_file_name()
                if len(cppFileName) > 0:
                    shutil.copyfile(compile_result.get_cpp_file_path(), tmp_dir + '/' +  cppFileName)

            with open(tmp_dir + '/' + 'compile_result.json', 'w') as f:
                f.write(compile_result.to_json())

            shutil.make_archive(output_dir + '/' + file_name, 'zip', tmp_dir)
        return output_dir + '/' + file_name + '.zip'
    
    def save(self, compile_result: FHECompileResult, output_dir: str) -> (str, str):
        """
        Save files generated during compilation into the zip format file

        Args
            compile_result (FHECompileResult)
                The collection of files generated during compilation
            output_dir (str):
                The location where the generated zip format file will be put

        Returns
            (str, str): return the client archive file path and the server archive file path
        """
        client_archive_path = self._do_save(compile_result, output_dir, False)
        server_archive_path = self._do_save(compile_result, output_dir, True)
        return (client_archive_path, server_archive_path)

    def load(self, archive_path: str) -> FHECompileResult:
        """
        Load the zip format file and return as compile result

        Args
            archive_path (str):
                The archive file path where the archive file will be loaded

        Returns
            FHECompileResult: return the compile result
        """
        tmp_dir = tempfile.mkdtemp()
        shutil.unpack_archive(archive_path, tmp_dir, 'zip')
        with open(tmp_dir + '/' + 'compile_result.json', 'r') as f:
            content = f.read()
        self._compile_result = FHECompileResult.from_json(content)
        self._compile_result.set_output_dir_path(tmp_dir)
        return self._compile_result

    def compile(self, onnx_file_or_py_function: Union[str, Callable], param_annos: Optional[Dict[str, str]] = None, compile_option: CompileOption = None) -> FHECompileResult:
        """
        Compile onnx file or python code into FHE operations

        Args
            onnx_file_or_py_function (Union[str, Callable]):
                Accept either
                    - str: the onnx file path
                    - Callable: the python function
            param_annos (Optional[Dict[str, str]]):
                Annotation for params, either encrypted or clear
            compile_option (CompileOption):
                options for compilation

        Raises
            RuntimeError: if neither onnx file nor python function is provided

        Returns
            FHECompileResult: return the compile result
        """
        if compile_option == None:
            compile_option = CompileOption()
            compile_option.compileTarget = COMPILE_TARGET.LIBRARY
            compile_option.outputDir = os.getenv('AEGIS_OUTPUT_DIR', "./output")
        self._output_dir = compile_option.outputDir
        if isinstance(onnx_file_or_py_function, str):
            mlir_file = self._convert_onnx_to_mlir(onnx_file_or_py_function)
        elif callable(onnx_file_or_py_function):
            mlir_file = self._convert_py_to_mlir(self._output_dir, onnx_file_or_py_function)
        else:
            raise RuntimeError("onnx_file and py_function are None")

        if param_annos != None:
            param_names = None
            if callable(onnx_file_or_py_function):
                source = inspect.getsource(onnx_file_or_py_function)
                match_args = re.search('\((.*?)\)', source)
                args_str = match_args.group(1)
            
                param_names = re.findall('(\w+)\s*:', args_str)
            self._modify_mlir_file(param_annos, param_names, mlir_file)

        self._compile_result = self._compiler.compile(mlir_file, compile_option)
        return self._compile_result

    def run(self, private_data: Union[bytes, List[bytes], Value, List[Value]]) -> Union[bytes, List[bytes], Value, List[Value]]:
        """
        Execute the FHE computation

        Args
            private_data (Union[bytes, List[bytes], Value, List[Value]]):
                The input data for computation

        Raises
            RuntimeError: if the server archive file is not loaded

        Returns
            Union[bytes, List[bytes], Value, List[Value]]: the computation result
        """
        self._require_keys_loaded()
        if self._compile_result == None:
            raise RuntimeError("server.zip is not provided")

        is_input_serialized = False
        if isinstance(private_data, bytes):
            private_data = Value.from_bytes(private_data)
            is_input_serialized = True
        elif isinstance(private_data, List) and isinstance(private_data[0], bytes):
            private_data = [Value.from_bytes(data) for data in private_data]
            is_input_serialized = True

        output = self._runtime.run(private_data, self._compile_result)

        if not is_input_serialized:
            return output

        if isinstance(output, Value):
            ser_value = output.to_bytes()
        else:
            ser_value = [o.to_bytes() for o in output]

        return ser_value

