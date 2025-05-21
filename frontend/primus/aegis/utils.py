import os
import importlib.resources


def ensure_bin_in_path():
    """
    Add primus/bin to PATH
    """

    def __valid_check(bin_path: str):
        if not os.path.isfile(bin_path):
            raise FileNotFoundError(f"Not found at: {bin_path}")
        if not os.access(bin_path, os.X_OK):
            raise PermissionError(f"Not executable: {bin_path}")

    try:
        # Get the primus/bin path
        bin_path = importlib.resources.files("primus").joinpath("bin")
        bin_path_str = str(bin_path)

        # uncomment for some checks
        # __valid_check(bin_path_str + "/mlir-translate")
        # __valid_check(bin_path_str + "/onnx-mlir")

        # Prepend to PATH
        current_path = os.environ.get("PATH", "")
        path_entries = current_path.split(os.pathsep)
        if bin_path_str not in path_entries:
            os.environ["PATH"] = bin_path_str + os.pathsep + current_path
    except ModuleNotFoundError:
        raise ImportError("Package 'primus' not found. Make sure it's installed.")
    except Exception as e:
        raise RuntimeError(f"Failed to add primus/bin to PATH: {e}")
