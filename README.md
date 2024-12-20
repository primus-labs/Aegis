# Aegis

## Overview

Aegis is an open-source framework based on MLIR that simplifies the use of Fully Homomorphic Encryption (FHE).

Aegis provides developer-friendly FHE compiler and runtime environment toolkit, with high-performance cryptography algorithms integration, easy to use APIs, aautomated tuning, and one-click deployment. Aegis supports various AI applications like NLP, CV, and even LLMs. It enables the reuse of these AI models, allowing code to be converted into a privacy-preserving form with minimal changes.

## Installation

### Prerequisites

#### Ubuntu

```shell
sudo apt update
sudo apt install clang
sudo apt install python3
sudo apt install python3-pip
sudo pip install torch          
sudo pip install tensorflow     # (Optional)
sudo pip install scikit-learn   # (Optional)
```

#### CentOS

```shell
sudo yum update
sudo yum install clang
sudo yum install python3
sudo yum install python3-pip
sudo pip install torch          
sudo pip install tensorflow     # (Optional)
sudo pip install scikit-learn   # (Optional)
```

> python version 3.10 or above (which can be checked by running python --version).

### Installation

#### Using Pip

Aegis offers out-of-the-box functionality, allowing you to get started quickly with a simple `python pip` installation.

Install Aegis using the following commands:

```shell
pip install -U pip wheel setuptools
pip install aegis
```

#### Using Docker

Aegis will offer Docker images in the future, which you can obtain and use with the following commands:

```shell
docker pull primus/aegis:v1.0.0
docker run --rm -it primus/aegis /bin/bash
```

## How Aegis Works

<img src='docs/_static/figs/concept.png' width = "850" height = "210" align="middle"/>

Aegis accepts pre-trained machine learning ONNX models as input, converts them to MLIR using the ONNX-MLIR integrated into the Aegis frontend, and then compiles the MLIR into a fully homomorphic encryption (FHE) program using Aegis. The program is executed through Aegis FHERuntime. This process enables Aegis to support pre-trained machine learning models that have been trained and fine-tuned using popular machine learning frameworks, such as PyTorch, TensorFlow, Scikit-learn, and more.

## Getting Started
TODO.

## Resources
TODO.

## Contributing
TODO.

## License
[Apache License 2.0](LICENSE)