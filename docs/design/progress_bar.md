## Overview
The progress bar can be shown to tell users the status during executing the computation.

### Modification in output.cpp
```cpp
#include <functional>
static function<void(const string& msg)> progress_bar_callback;

void setProgressBarCallback(function<void(const string& msg)> callback) {
    progress_bar_callback = callback;
}
```

### ProgressBar
```cpp
class ProgressBar {
private:
    int totalCount = 0;
    int current = 0;

private:
    ProgressBar() = default;

public:
    static ProgressBar& instance() {
        static ProgressBar bar;
        return bar;
    }

    void setTotalCount(int count) {
        totalCount = count;
        current = 0;
    }
    
    void updateProgress(const string& msg);
};

void updateProgressBar(const string& msg) {
    ProgressBar::instance().updateProgress(msg);
}
```

### FHERuntime
```cpp
class FHERuntime {
public:
    void initProgressBar(const string& prog_spec_content) {
        // parse prog_spec_content
        ProgressBar::instance().setTotalCount(...);
    }
};
```

### Binding
```cpp
#include <pybind11/pybind11.h>
static pybind11::function python_function;
void set_python_function(pybind11::function fn) {
    python_function = fn;
}

void call_python_function(const string& msg) {
    if (python_function) {
        python_function(msg);
    }
}

PYBIND11_MODULE(callback, m) {
    m.def("set_print_callback", &set_python_function);
}
```

### Python
```python
def print_msg(msg):
    print(msg, end = '', flush = True)
```
