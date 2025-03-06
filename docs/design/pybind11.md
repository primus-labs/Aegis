

## Modules

- The module name is `primus.aegis_c`.
- The default backend is CPU. (Maybe add .gpu for GPU backend, such as` primus.aegis_c.gpu.XXX`.)
- The source folder is `./frontend/cc`.
- The way how to import the module:
  ```py
  import primus.aegis_c
  ```



### FHEKey


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

    +decrypt(Vector~uint8_t~) Vector~double~
  }
  note for FHEPrivateKey "the date-type of decrypt??"

  IFHEKey <|.. FHEPublicKey
  class FHEPublicKey {
    +serialize() string
    +deserialize(string)

    +encrypt(Vector~T~) Vector~uint8_t~
  }
  note for FHEPublicKey "the date-type of encrypt??"

  IFHEKey <|.. FHERelinKey
  class FHERelinKey {
    +serialize() string
    +deserialize(string)
  }

  IFHEKey <|.. FHERotateKey
  class FHERotateKey {
    +serialize() string
    +deserialize(string)
  }

  IFHEKey <|.. FHEBootstrapKey
  class FHEBootstrapKey {
    +serialize() string
    +deserialize(string)
  }
```

```mermaid {align="center"}
classDiagram
  class FHEKeyInfo {
    +polyModDegree : UInt32       // poly modulus degree.
    +coffModCh : List~UInt32~     // coff modulus chain.
    +scale : UInt32               // scale factor.
    +multDepth : UInt32           // Multiplication depth
    +scaleModSize : UInt32        // Scale modulus size
    +batchSize : UInt32           // Batch size
    +galoisIndices : List~Int32~  // Index list for Galois Key
    +enableBootstrapping : Bool   // Whether to enable bootstrapping
    +numSlot : UInt32             // number of slots
  }
```

```mermaid {align="center"}
classDiagram
  class FHEKeyset {
    +generate(FHEKeyInfo) FHEKeyset$
    +initialize(FHEPrivateKey, FHEPublicKey,FHERelinKey, FHERotateKey,FHEBootstrapKey) FHEKeyset$
    +getPrivateKey() FHEPrivateKey
    +getPublicKey() FHEPublicKey
    +getRelinKey() FHERelinKey
    +getRotateKey() FHERotateKey
    +getBootstrapKey() FHEBootstrapKey
  }
```

**Simple usage:** (expected)

```py
from primus.aegis_c import FHEKeyInfo, FHEKeyset, ...

# set the parameters of FHEKeyInfo
fheKeyInfo = FHEKeyInfo()
fheKeyInfo.xxx = yyy

# generate keys
fheKeyset = FHEKeyset.generate(fheKeyInfo)

# get one or some key(s) by FHEKeyset.getXXXKey()
fhePrivateKey = fheKeyset.getPrivateKey()
fhePublicKey = fheKeyset.getPublicKey()

# do some operations
plainData = ...
encryptedData = fhePublicKey.encypt(plainData)
decryptedData = fhePrivateKey.decypt(encryptedData)
```

### DataProcessor

```mermaid {align="center"}
classDiagram
  class Utils {
    +PyValue2Value(Vector~np.double~) Vector~Value~$
    +Value2RawData(Vector~Value~) Vector~RawData~$
    +RawData2PyValue(Vector~RawData~) Vector~np.double~$
  }
  note for Utils "internal tools"
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
    -_processOutput(Vector~np.uint8~) Vector~np.double~
  }
  
  DataProcessor <|.. FHEDataProcessor
  class FHEDataProcessor {
    +privateInput(Vector~np.double~) Vector~np.uint8~
    +publicInput(Vector~np.double~) Vector~np.uint8~
    +processOutput(Vector~np.uint8~) Vector~np.double~
  }
```

- Before privateInput: `numpy(double) -> Tensor(double) -> Value(double)`
- At privateInput: `Value(double) -> Tensor(double) -> encrypt() --> Tensor(uint8_t) -> Value(uint8_t)`
- At processOutput: `Value(uint8_t) -> Tensor(uint8_t) -> decrypt() --> Tensor(double) -> Value(double)`



**Simple usage:** (expected)

```py
from primus.aegis_c import FHEDataProcessor

plainInputData = numpy<np.double>

fheFHEDataProcessor = FHEDataProcessor()
privateData = fheFHEDataProcessor.privateInput(plainInputData)

```

### Runtime


```mermaid {align="center"}
classDiagram
  class BACKEND_TYPE{
    <<enumeration>>
    CPU
    GPU
  }
  class COMPILE_MODE{
    <<enumeration>>
    COMPILE
    TRANSPILER
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
    backendType : BACKEND_TYPE = BACKEND_TYPE::CPU
    compileMode : COMPILE_MODE = COMPILE_MODE::COMPILE
  }
  class CompileResult{
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
  note for Runtime "the input's meaning of run??"
  
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



**Simple usage:** (expected)

```py
from primus.aegis_c import CompileOption, FHERuntime, ...

mlir_file = "/path/to/xxx.mlir"
compile_option = CompileOption()

fheRuntime = FHERuntime()

# compile
compile_result = fheRuntime.compile(mlir_file, compile_option)

# run
inputData = ...
run_result = fheRuntime.run(inputData, compile_result)
# what's inputData?? run_result??
```
