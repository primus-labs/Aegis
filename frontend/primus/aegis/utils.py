import os
import importlib.resources
import ctypes
import platform


def __valid_check(bin_path: str):
    if not os.path.isfile(bin_path):
        raise FileNotFoundError(f"Not found at: {bin_path}")
    if not os.access(bin_path, os.X_OK):
        raise PermissionError(f"Not executable: {bin_path}")


def load_dependencis():
    try:
        system = platform.system()
        so_names = ["libmlir_float16_utils.so.20.0git", "libmlir_runner_utils.so", "libAegisRuntime.so.20.0git"]
        if system == "Darwin":
            so_names = ["libmlir_float16_utils.dylib", "libmlir_runner_utils.dylib", "libAegisRuntime.dylib"]
        for so_name in so_names:
            lib_path = importlib.resources.files("primus").joinpath(f"lib/{so_name}")
            lib_path_str = str(lib_path)
            __valid_check(lib_path_str)
            ctypes.CDLL(lib_path_str)
    except ModuleNotFoundError:
        raise ImportError("Package 'primus' not found. Make sure it's installed.")
    except Exception as e:
        raise RuntimeError(f"Failed to load dependencis library: {e}")


def ensure_bin_in_path():
    """
    Add primus/bin to PATH
    """
    try:
        # Get the primus/bin path
        bin_path = importlib.resources.files("primus").joinpath("bin")
        bin_path_str = str(bin_path)

        # some checks
        __valid_check(bin_path_str + "/mlir-translate")
        __valid_check(bin_path_str + "/onnx-mlir")

        # Prepend to PATH
        current_path = os.environ.get("PATH", "")
        path_entries = current_path.split(os.pathsep)
        if bin_path_str not in path_entries:
            os.environ["PATH"] = bin_path_str + os.pathsep + current_path
    except ModuleNotFoundError:
        raise ImportError("Package 'primus' not found. Make sure it's installed.")
    except Exception as e:
        raise RuntimeError(f"Failed to add primus/bin to PATH: {e}")
