#!/bin/bash


# =============================================================================
# Set permanent PYTHONPATH for MLIR development on Linux and macOS
# =============================================================================
# Target Python path to be added
TARGET_PATH="/usr/local/python_packages/mlir_core"

# =============================================================================
# Detect operating system and current shell
# =============================================================================
OS_TYPE="unknown"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS_TYPE="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS_TYPE="macos"
fi

# Detect current shell type
if [ -n "$BASH_VERSION" ]; then
    SHELL_TYPE="bash"
    CONFIG_FILE="$HOME/.bashrc"
elif [ -n "$ZSH_VERSION" ]; then
    SHELL_TYPE="zsh"
    CONFIG_FILE="$HOME/.zshrc"
else
    SHELL_TYPE=$(basename "$SHELL")
    if [ "$SHELL_TYPE" = "bash" ]; then
        CONFIG_FILE="$HOME/.bashrc"
    elif [ "$SHELL_TYPE" = "zsh" ]; then
        CONFIG_FILE="$HOME/.zshrc"
    fi
fi

# Adjust configuration file path for macOS bash users
if [[ "$OS_TYPE" == "macos" && "$SHELL_TYPE" == "bash" ]]; then
    CONFIG_FILE="$HOME/.bash_profile"
fi

echo "Detection results:"
echo "  - OS: $OS_TYPE"
echo "  - Shell: $SHELL_TYPE"
echo "  - Config file: $CONFIG_FILE"

# =============================================================================
# Prepare configuration file
# =============================================================================
# Create config file if doesn't exist
if [ ! -f "$CONFIG_FILE" ]; then
    echo "Config file not found, creating..."
    touch "$CONFIG_FILE"
    echo "#!/bin/$SHELL_TYPE" >> "$CONFIG_FILE"
    echo "# User configuration for $SHELL_TYPE" >> "$CONFIG_FILE"
    echo "# Created by PYTHONPATH setup script on $(date)" >> "$CONFIG_FILE"
    echo -e "\n" >> "$CONFIG_FILE"
fi

# =============================================================================
# Add PYTHONPATH to user profile
# =============================================================================
# Define the export statement
EXPORT_STATEMENT="export PYTHONPATH=\"${TARGET_PATH}:\$PYTHONPATH\""

# Check if path already exists to prevent duplicates
grep -qF -- "PYTHONPATH=\"${TARGET_PATH}:" "$CONFIG_FILE" 2>/dev/null
if [ $? -ne 0 ]; then
    echo -e "\n# ======= MLIR Python path settings - START =======" >> "$CONFIG_FILE"
    echo "# Added by PYTHONPATH setup script on $(date)" >> "$CONFIG_FILE"
    echo "# This setting adds MLIR core Python bindings to PYTHONPATH" >> "$CONFIG_FILE"
    echo "$EXPORT_STATEMENT" >> "$CONFIG_FILE"
    echo "# ======= MLIR Python path settings - END =======" >> "$CONFIG_FILE"
    echo -e "PYTHONPATH configuration added to: $CONFIG_FILE"
else
    echo "PYTHONPATH settings already exist in config file: $CONFIG_FILE"
fi

# =============================================================================
# Apply changes to current session
# =============================================================================
# Refresh current shell configuration
if [ -f "$CONFIG_FILE" ]; then
    if [ "$SHELL_TYPE" = "zsh" ]; then
        source "$CONFIG_FILE" > /dev/null 2>&1 && echo "Zsh config refreshed"
    else
        source "$CONFIG_FILE" > /dev/null 2>&1 && echo "Bash config refreshed"
    fi
fi

echo -e "\nSetup PYTHONPATH completed successfully!"