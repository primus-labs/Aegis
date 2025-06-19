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

