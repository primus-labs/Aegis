

- [Scripts](#scripts)
  - [test\_binding.sh](#test_bindingsh)
  - [copy\_files\_to\_frontend.sh](#copy_files_to_frontendsh)
  - [build\_wheel.sh](#build_wheelsh)



## Scripts

### test_binding.sh

- Test binding level


### copy_files_to_frontend.sh

Use `./scripts/copy_files_to_frontend.sh` to copy the necessary files to the following path for packaging with frontend:

```log 
./frontend/primus/
├── bin
│   ├── mlir-translate
│   └── onnx-mlir
└── lib
    ├── libAegisRuntime.so*
    └── primus_aegis.so
```

**Note**: Once you have the relevant files under `bin` and `lib`, you don't need to set the corresponding paths in the test script.


### build_wheel.sh

Use `./scripts/build_wheel.sh` to package the frontend into the `dist/` folder:

```log
./frontend/
├── dist
│   └── primus-0.1.0-py3-none-any.whl
```

**Note**: To execute this packaging script, you need to install python `3.10+` and do the following installation:

```sh 
sudo pip install build 
sudo apt install python3.10-venv # corresponding to the python version 
```
