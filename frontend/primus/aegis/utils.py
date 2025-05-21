import os
import importlib.resources
import importlib.util
import importlib.machinery


def __valid_check(bin_path: str):
    if not os.path.isfile(bin_path):
        raise FileNotFoundError(f"Not found at: {bin_path}")
    if not os.access(bin_path, os.X_OK):
        raise PermissionError(f"Not executable: {bin_path}")


def load_primus_aegis():
    try:
        lib_path = importlib.resources.files("primus").joinpath("lib/primus_aegis.so")
        lib_path_str = str(lib_path)
        __valid_check(lib_path_str)

        loader = importlib.machinery.ExtensionFileLoader("primus_aegis", str(lib_path_str))
        spec = importlib.util.spec_from_loader("primus_aegis", loader)
        module = importlib.util.module_from_spec(spec)
        loader.exec_module(module)
        return module
    except ModuleNotFoundError:
        raise ImportError("Package 'primus' not found. Make sure it's installed.")
    except Exception as e:
        raise RuntimeError(f"Failed to load primus_aegis: {e}")


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
