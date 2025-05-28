import os
from .py2mlir import MLIRGenerator
from mlir import ir
import subprocess
import inspect
import ast
import re
from typing import Callable, Tuple, List

class Py2MLIRConverter:
    def __init__(self, output_dir: str):
        self.output_dir = output_dir

    def pythontomlir(self, functionstr: str, mlir_output_path: str):
        """
        Converts Python function to MLIR format using the heir-opt tool.
        
        Args:
            functionstr (str): String representation of the Python function.
            mlir_output_path (str): Path to the output MLIR file.
        """
        parsed_ast = ast.parse(functionstr)
        os.makedirs(os.path.dirname(mlir_output_path), exist_ok=True)
        with ir.Context() as ctx:
            with ir.Location.unknown():
                module = ir.Module.create()
                generator = MLIRGenerator(module)
                generator.visit(parsed_ast)
                with open(mlir_output_path, 'w') as mlir_file:
                    mlir_file.write(str(module))

    def convert(self, function: Callable) -> str:
        """
        Compiles a given function.
        
        Args:
            function (Callable): The function to be compiled.

        Returns:
            Tuple[str, List[int]]: A tuple containing the path to the compiled C++ file and a list of rotate steps.
        """
        function_name = function.__name__
        source_code = inspect.getsource(function)

        mlir_output_path = os.path.join(self.output_dir, f'{function_name}.mlir')
        self.pythontomlir(source_code, mlir_output_path)
        return mlir_output_path

