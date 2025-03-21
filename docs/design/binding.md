---
version 3: 2025.03.14
version 2: 2025.03.07
version 1: 2025.03.05
---

- [Overview](#overview)
- [FHE](#fhe)
  - [Simple Usage](#simple-usage)
- [DataProcessor](#dataprocessor)
  - [Simple Usage](#simple-usage-1)
- [Runtime](#runtime)
  - [Simple Usage](#simple-usage-2)


## Overview

- The module name is `primus.aegis_c`.
- The default backend is CPU. (Maybe add .gpu for GPU backend, such as` primus.aegis_c.gpu.XXX`.)
- The source folder is `./midend/lib/Binding`.
- The way how to import the module:
  ```py
  import primus.aegis_c
  ```

<br/>

Basic flow:
- Run `onnx-mlir` to compile the `onnx-model` to generate the `model.mlir` file.
- Call `Runtime.compile(model.mlir)` to generate the `program_spec.json` file.
- Call `FHEKeyset.generate(program_spec.json)` to generate the FHE keys and context.
- Call `Runtime.run(input)` and so on.


## FHE

**FHEKeys**


```mermaid {align="center"}
classDiagram
  class IFHEKey {
    <<interface>>
    +serialize() string
    +deserialize(string)
  }

  IFHEKey <|.. FHEPrivateKey
  class FHEPrivateKey {
    +serialize() string
    +deserialize(string)
  }

  IFHEKey <|.. FHEPublicKey
  class FHEPublicKey {
    +serialize() string
    +deserialize(string)
  }
  %% C++ backend for XXXKey should provider a default constructor
```
Some keys related to FHE. The serialize/deserialize methods make it easy to save/load these keys.

**FHEKeyset**


```mermaid {align="center"}
classDiagram
  class FHEKeyset {
    +generate(spec.json: string) FHEKeyset$
    +loads(string) FHEKeyset$
    +dumps(contain_sk: boolean = true) string
    +getPrivateKey() FHEPrivateKey
    +getPublicKey() FHEPublicKey
  }
```

- Use `.generate()` to generate a set of keys (and context) by passing in the `program_sepc.json` file.
- Use `.get***Key()` to get the corresponding key.
- Use `.dumps()` to save all the keys to a string, and then use `.loads()` to load the saved keys. Only publicly available information will be exported if set `contain_sk` to `false`.



### Simple Usage

```py
from primus.aegis_c import FHEKeyset

# generate keys
fheKeyset = FHEKeyset.generate(program_spec.json)

# serialize all keys(and context) to a string
all_keys = fheKeyset.dumps()

# also can only dump publicly information by setting `contain_sk` to false.
# and send to compute node
publicly_keys = fheKeyset.dumps(false)

# load the keys
fheKeyset = FHEKeyset.loads(all_keys)
# or in the compute node
fheKeyset = FHEKeyset.loads(publicly_keys)


# you can only export private key or public key
fhePrivateKey = fheKeyset.getPrivateKey()
fhePublicKey = fheKeyset.getPublicKey()
private_key = fhePrivateKey.serialize()
public_key = fhePublicKey.serialize()
```


## DataProcessor

```mermaid {align="center"}
classDiagram
  class Utils {
    <<internal toolkit>>
    +PyValue2Value(Vector~np.double~) Vector~Value~$
    +Value2RawData(Vector~Value~) Vector~RawData~$
    +RawData2PyValue(Vector~RawData~) Vector~np.double~$
  }
```

```mermaid {align="center"}
classDiagram
  class DataProcessor {
    <<abstract>>
    +privateInput(Vector~np.double~) Vector~np.uint8~
    +publicInput(Vector~np.double~) Vector~np.uint8~
    +processOutput(Vector~np.uint8~) Vector~np.double~

    -_privateInput(Vector~Value~) Vector~Value~
    -_publicInput(Vector~Value~) Vector~Value~
    -_processOutput(Vector~Value~) Vector~Value~
  }
  
  DataProcessor <|.. FHEDataProcessor
  class FHEDataProcessor {
    +privateInput()
    +publicInput()
    +processOutput()
  }
  
  DataProcessor <|.. MPCDataProcessor
  class MPCDataProcessor {
    +privateInput()
    +publicInput()
    +processOutput()
  }
  
  DataProcessor <|.. ZKDataProcessor
  class ZKDataProcessor {
    +privateInput()
    +publicInput()
    +processOutput()
  }
```
The `Utils` is the internal toolkit only used in binding level.
The data-type flow of the DataProcessor:
- Before privateInput: `numpy(double) -> Tensor(double) -> Value(double)`
- At privateInput: `Value(double) -> Tensor(double) -> encrypt() --> Tensor(uint8_t) -> Value(uint8_t)`
- At processOutput: `Value(uint8_t) -> Tensor(uint8_t) -> decrypt() --> Tensor(double) -> Value(double)`

### Simple Usage

```py
from primus.aegis_c import FHEDataProcessor

plainInputData = <np.double>

fheFHEDataProcessor = FHEDataProcessor()
privateData = fheFHEDataProcessor.privateInput(plainInputData)
```

## Runtime


```mermaid {align="center"}
classDiagram
  class BACKEND_TYPE{
    <<enumeration>>
    CPU
    GPU
  }
  class COMPILE_TARGET{
    <<enumeration>>
    SECRET
    FHE
    EMITC
    CPP 
    LIBRARY
  }
```

```mermaid {align="center"}
classDiagram
  class CompileOption{
    <<struct>>
    backendType : BACKEND_TYPE = BACKEND_TYPE::CPU
    compileTarget : COMPILE_TARGET = COMPILE_TARGET::SECRET
  }
  class CompileResult{
    <<struct>>
    outputDirPath : string
    cppFileName : string
    binFileName : string
    progSpecFileName : string
  }
```


```mermaid {align="center"}
classDiagram
  class Runtime {
    <<abstract>>
    +compile(mlir_file:string, CompileOption) CompileResult
    +run(input:Vector~np.double~, CompileResult) Vector~np.uint8~
    -_run(input:Vector~Value~, CompileResult) Vector~Value~
  }

  Runtime <|.. FHERuntime
  class FHERuntime {
    +compile()
    +run()
  }
  Runtime <|.. MPCRuntime
  class MPCRuntime {
    +compile()
    +run()
  }
  Runtime <|.. ZKPRuntime
  class ZKPRuntime {
    +compile()
    +run()
  }
```
- The `mlir_file`, the first parameter of `.compile()`, is generated by `onnx-mlir` after compiling the `onnx-model`.
- The `CompileResult.progSpecFileName` is the input to `Keyset.generate()`.

### Simple Usage

```py
from primus.aegis_c import CompileOption, FHERuntime, ...

mlir_file = "/path/to/xxx.mlir"
compile_option = CompileOption()

fheRuntime = FHERuntime()

# compile
compile_result = fheRuntime.compile(mlir_file, compile_option)
# construct Keyset by `Keyset.generate(compile_result.progSpecFileName)`

# run
plainInputData = <np.double>
run_result = fheRuntime.run(inputData, compile_result)

# get the plain result
fheFHEDataProcessor = FHEDataProcessor()
plain_result = fheFHEDataProcessor.processOutput(run_result)
```

