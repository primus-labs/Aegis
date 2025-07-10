#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;

#include "Common/ProgramSpec.h"
#include "Common/Protocol.h"
#include "Common/Value.h"
#include "CryptoContextMgr.h"
#include "Runtime/CompilerEngine.h"
#include "Runtime/FHE/FHEDataProcessor.h"
#include "Runtime/FHE/FHERuntime.h"
#include "Runtime/Simulate/SimDataProcessor.h"
#include "Runtime/Simulate/SimRuntime.h"
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <capnp/serialize.h>
#include <protocol.capnp.h>
using namespace mlir::aegis;

#include "FheKey.h"
#include "KeysetGenerator.h"
#include "Operate.h"
using namespace aegiscpu;
using namespace openfhe;

// NOTE!!! MUST INCLUDE THE FOLLOWING HEADERS
// header files needed for serialization
#include "ciphertext-ser.h"
#include "cryptocontext-ser.h"
#include "key/key-ser.h"
#include "scheme/ckksrns/ckksrns-ser.h"
using namespace lbcrypto;

#include <iostream>
#include <sstream>
using namespace std;

#include <nlohmann/json.hpp>

namespace mlir::aegis {
void to_json(nlohmann::json &j, const CompileOptions &x) {
    j = nlohmann::json{{"backendType", x.beType},
                       {"compileTarget", x.target},
                       {"scheme", x.scheme},
                       {"verbose", x.verbose},
                       {"outputDir", x.outputDir}};
}
void from_json(const nlohmann::json &j, CompileOptions &x) {
    j.at("backendType").get_to(x.beType);
    j.at("compileTarget").get_to(x.target);
    j.at("scheme").get_to(x.scheme);
    j.at("verbose").get_to(x.verbose);
    j.at("outputDir").get_to(x.outputDir);
}
void to_json(nlohmann::json &j, const CompileResult &x) {
    j = nlohmann::json{{"outputDirPath", x.outputDirPath},
                       {"cppFileName", x.cppFileName},
                       {"simFileName", x.simFileName},
                       {"binFileName", x.binFileName},
                       {"progSpecFileName", x.progSpecFileName}};
}
void from_json(const nlohmann::json &j, CompileResult &x) {
    j.at("outputDirPath").get_to(x.outputDirPath);
    j.at("cppFileName").get_to(x.cppFileName);
    j.at("simFileName").get_to(x.simFileName);
    j.at("binFileName").get_to(x.binFileName);
    j.at("progSpecFileName").get_to(x.progSpecFileName);
}
} // namespace mlir::aegis

using AegisValue = mlir::aegis::Value;
struct PyValue {
    uint32_t ndim;
    std::vector<AegisValue> values;
};

/// @brief old
struct KeyInfo {
    uint32_t polyModDegree;
    std::vector<uint32_t> coffModCh;
    uint32_t scale;
    uint32_t multDepth;
    uint32_t firstModSize;
    uint32_t scaleModSize;
    uint32_t batchSize;
    std::vector<int32_t> galoisIndices;
    bool enableBootstrapping;
    std::string scheme;
};

/// @brief old
static void makeProtoKeyInfo(const KeyInfo &keyInfo, ProtoMessage<aegisprotocol::KeyInfo> &protoKeyInfo) {
    protoKeyInfo.asBuilder().setPolyModDegree(keyInfo.polyModDegree);
    protoKeyInfo.asBuilder().setScale(keyInfo.scale);
    protoKeyInfo.asBuilder().setMultDepth(keyInfo.multDepth);
    protoKeyInfo.asBuilder().setFirstModSize(keyInfo.firstModSize);
    protoKeyInfo.asBuilder().setScaleModSize(keyInfo.scaleModSize);
    protoKeyInfo.asBuilder().setBatchSize(keyInfo.batchSize);
    protoKeyInfo.asBuilder().setEnableBootstrapping(keyInfo.enableBootstrapping);
    protoKeyInfo.asBuilder().setScheme(keyInfo.scheme);

    auto coff = keyInfo.coffModCh;
    auto coffModCh = protoKeyInfo.asBuilder().initCoffModCh(coff.size());
    for (size_t i = 0; i < coff.size(); ++i) {
        coffModCh.set(i, coff[i]);
    }
    auto galois = keyInfo.galoisIndices;
    auto galoisIndices = protoKeyInfo.asBuilder().initGaloisIndices(galois.size());
    for (size_t i = 0; i < galois.size(); ++i) {
        galoisIndices.set(i, galois[i]);
    }
}

/// @brief old
static void generate_keyset_old(FheKeyset &self, const KeyInfo &keyInfo) {
    static std::once_flag initFlag;
    std::call_once(initFlag, [&]() {
        ProtoMessage<aegisprotocol::KeyInfo> protoKeyInfo;
        makeProtoKeyInfo(keyInfo, protoKeyInfo);

        // initialize
        KeysetGenerator::generate(protoKeyInfo);
    });
}

/// @brief
static void generate_keyset(FheKeyset &self, const std::string &prog_spec_file) {
    ProgramSpec &progSpecObj = ProgramSpec::getInstance();
    if (!progSpecObj.initialize(prog_spec_file)) {
        throw std::runtime_error("Cannot initialize ProgramSpec");
    }

    static std::once_flag initFlag;
    std::call_once(initFlag, [&]() {
        ProtoMessage<aegisprotocol::KeyInfo> keyInfo = progSpecObj.getKeyInfo();

        // initialize
        KeysetGenerator::generate(keyInfo);
    });
}

std::vector<std::vector<double>> reshape2d(const std::vector<double> &input, size_t m, size_t n) {
    if (input.size() != m * n)
        throw std::invalid_argument("reshape size mismatch");

    std::vector<std::vector<double>> result;
    result.reserve(m);
    for (size_t i = 0; i < m; ++i) {
        result.emplace_back(input.begin() + i * n, input.begin() + (i + 1) * n);
    }
    return result;
}

/// @brief
class Utils {
  public:
    /**
     * Convert numpy.array(o, dtype=np.float64) to Value(double)
     */
    static Value Numpy2Value(const py::array_t<double> &input) {
        auto buf = input.request();
#ifndef NDEBUG
        {
            std::stringstream ss;
            ss << "itemsize: " << buf.itemsize << ", size: " << buf.size << ", format: " << buf.format
               << ", ndim: " << buf.ndim << ", shape: (";
            for (size_t i = 0; i < buf.shape.size(); ++i) {
                ss << buf.shape[i];
                if (i < buf.shape.size() - 1)
                    ss << ", ";
            }
            ss << ")";
            std::cout << ss.str() << std::endl;
        }
#endif

        std::vector<size_t> shapes;
        for (ssize_t v : buf.shape) {
            shapes.push_back(v);
        }
        double *ptr = static_cast<double *>(buf.ptr);
        auto values = std::vector<double>(ptr, ptr + buf.size);
        Tensor<double> tensor(std::move(values), shapes);
        return Value(std::move(tensor));
    }
    /**
     * Convert Value(double) to numpy.array(o, dtype=np.float64)
     */
    static py::array_t<double> Value2Numpy(const Value &input) {
        auto tensor = input.getTensor<double>().value(); // TODO: should add getValues()/getDims() for Tensor
        auto values = tensor.values;
        auto dims = tensor.dims;
#ifndef NDEBUG
        cout << "dims.size() " << dims.size() << endl;
        cout << "values " << values.size() << endl;
#endif
        return py::array_t<double>(dims, values.data());
    }

    /**
     * Convert Value(uint8_t) to py::bytes
     */
    static py::bytes Value2PyBytes(const Value &input) {
        auto tensor = input.getTensor<uint8_t>().value(); // TODO: should add getValues()/getDims() for Tensor
        auto values = tensor.values;
        auto dims = tensor.dims;

        //
        // Payload
        ProtoMessage<aegisprotocol::Payload> protoPayload;
        protoPayload.asBuilder().setData(kj::arrayPtr(values.data(), values.size()));

        // Shape
        ProtoMessage<aegisprotocol::Shape> protoShape;
        auto dimensions = protoShape.asBuilder().initDimensions(dims.size());
        for (size_t i = 0; i < dims.size(); ++i) {
            dimensions.set(i, dims[i]);
        }

        // RawInfo
        ProtoMessage<aegisprotocol::RawInfo> protoRawInfo;
        protoRawInfo.asBuilder().setShape(protoShape.asReader());
        protoRawInfo.asBuilder().setIsSigned(false);

        // RawType
        ProtoMessage<aegisprotocol::RawType> protoRawType;
        protoRawType.asBuilder().setIsCiphertext(true);
        protoRawType.asBuilder().setIsPlaintext(false); // TODO: no need since have isCiphertext
        protoRawType.asBuilder().setIsConstant(false);

        // RawData
        ProtoMessage<aegisprotocol::RawData> protoRawData;
        protoRawData.asBuilder().setPayload(protoPayload.asReader());
        protoRawData.asBuilder().setRawInfo(protoRawInfo.asReader());
        protoRawData.asBuilder().setRawType(protoRawType.asReader());

        // Serialize to a byte array
        std::stringstream ss;
        protoRawData.writeBinaryToOstream(ss);
        return py::bytes(ss.str());
    }
    static py::bytes PyValue2PyBytes(const PyValue &pv) {
        std::ostringstream oss;
        oss.write(reinterpret_cast<const char *>(&pv.ndim), sizeof(uint32_t));

        uint32_t vsize = (uint32_t)pv.values.size();
        oss.write(reinterpret_cast<const char *>(&vsize), sizeof(uint32_t));

        for (const auto &v : pv.values) {
            auto b = Utils::Value2PyBytes(v);
            std::string s = b;
            uint32_t len = (uint32_t)s.size();
            oss.write(reinterpret_cast<const char *>(&len), sizeof(uint32_t));
            oss.write(s.data(), len);
        }
        return py::bytes(oss.str());
    }

    /**
     * Convert py::bytes to Value(uint8_t)
     */
    static Value PyBytes2Value(const py::bytes &input) {
        std::string data = input.cast<std::string>();
        std::stringstream ss(data);

        // Deserialize from a byte array
        ProtoMessage<aegisprotocol::RawData> protoRawData;
        protoRawData.readBinaryFromIstream(ss);

        // RawData
        auto protoPayload = protoRawData.asReader().getPayload();
        auto protoRawInfo = protoRawData.asReader().getRawInfo();
        auto protoRawType = protoRawData.asReader().getRawType();

        // RawType
        protoRawType.getIsCiphertext(); // TODO: Check
        protoRawType.getIsPlaintext();
        protoRawType.getIsConstant();

        // RawInfo
        auto protoShape = protoRawInfo.getShape();
        protoRawInfo.getIsSigned();

        // Shape
        auto dimensions = protoShape.getDimensions();
        vector<size_t> dims;
        for (size_t i = 0; i < dimensions.size(); ++i) {
            dims.push_back(dimensions[i]);
        }

        // Payload
        std::vector<uint8_t> values(protoPayload.getData().begin(), protoPayload.getData().end());

        Tensor<uint8_t> tensor(std::move(values), dims);
        return Value(std::move(tensor));
    }
    static PyValue PyBytes2PyValue(const py::bytes &b) {
        PyValue pv;
        std::string s = b;
        std::istringstream iss(s);
        iss.read(reinterpret_cast<char *>(&pv.ndim), sizeof(uint32_t));

        uint32_t vsize = (uint32_t)pv.values.size();
        iss.read(reinterpret_cast<char *>(&vsize), sizeof(uint32_t));

        for (size_t i = 0; i < (size_t)vsize; ++i) {
            uint32_t len;
            iss.read(reinterpret_cast<char *>(&len), sizeof(uint32_t));
            std::string s(len, '\0');
            iss.read(&s[0], len);
            auto v = Utils::PyBytes2Value(py::bytes(s));
            pv.values.push_back(v);
        }

        return pv;
    }
};

/// @brief
template <typename DataProcessor = FHEDataProcessor, bool IsSimualte = false> class PyDataProcessor {
  public:
    static PyValue privateInput(const py::array_t<double> &input) {
        PyValue pv;
        auto inputValue = Utils::Numpy2Value(input);
        auto tensor = inputValue.getTensor<double>().value();
        auto dims = tensor.dims;
        if (dims.size() == 0 || dims.size() == 1) {
            auto cipherValue = DataProcessor().privateInput(tensor.values);
            pv.ndim = 1;
            pv.values.push_back(cipherValue);
        } else if (dims.size() == 2) {
            // m x n (dims[0] x dims[1])
            auto values = reshape2d(tensor.values, dims[0], dims[1]);
            auto cipherValues = DataProcessor().privateInput(values);
            pv.ndim = 2;
            pv.values = cipherValues;
        } else {
            throw std::runtime_error("Invalid dims: only support 1-d or 2-d");
        }
        return pv;
    }

    static PyValue publicInput(const py::array_t<double> &input) {
        PyValue pv;
        auto inputValue = Utils::Numpy2Value(input);
        auto tensor = inputValue.getTensor<double>().value();
        auto dims = tensor.dims;
        if (dims.size() == 0) {
            if constexpr (IsSimualte) {
                auto cipherValue = DataProcessor().publicInput(tensor.values[0]);
                pv.ndim = 0;
                pv.values.push_back(cipherValue);
            } else {
                auto cipherValue = DataProcessor().publicInput(tensor.values);
                pv.ndim = 1;
                pv.values.push_back(cipherValue);
            }
        } else if (dims.size() == 1) {
            auto cipherValue = DataProcessor().publicInput(tensor.values);
            pv.ndim = 1;
            pv.values.push_back(cipherValue);
        } else if (dims.size() == 2) {
            // m x n (dims[0] x dims[1])
            auto values = reshape2d(tensor.values, dims[0], dims[1]);
            auto cipherValues = DataProcessor().publicInput(values);
            pv.ndim = 2;
            pv.values = cipherValues;
        } else {
            throw std::runtime_error("Invalid dims: only support 1-d or 2-d");
        }
        return pv;
    }

    static py::array_t<double> processOutput(PyValue &pv) { // TODO:const input
        if (pv.ndim == 1) {
            auto plainValues = DataProcessor().processOutput(pv.values);
            return Utils::Value2Numpy(plainValues[0]);
        } else if (pv.ndim == 2) {
            auto plainValues = DataProcessor().processOutput(pv.values);

            std::vector<size_t> shapes = {plainValues.size()}; // m:dims[0]
            std::vector<double> values;
            for (auto &input : plainValues) {
                auto tensor = input.template getTensor<double>().value();
                values.insert(values.end(), tensor.values.begin(), tensor.values.end());
            }
            shapes.push_back(values.size() / plainValues.size()); // n:dims[1]

            Tensor<double> tensor(std::move(values), shapes);
            return Utils::Value2Numpy(Value(std::move(tensor)));
        } else {
            throw std::runtime_error("Invalid ndim: only support 1-d or 2-d");
        }
    }
};

/// @brief
class PyCompiler {
  public:
    CompileResult compile(const std::string &mlir_content, const CompileOptions &compileOptions) {
        auto compile_context = CompileContext::createContext();
        CompilerEngine engine(compile_context);
        engine.setCompileOptions(compileOptions);
        auto result = engine.compile(mlir_content);
        if (!result) {
            auto s = llvm::toString(result.takeError());
            throw std::runtime_error(s);
        }

        return *result;
    }
};

/// @brief
class PyFHERuntime {
  public:
    vector<PyValue> run(const vector<PyValue> &pvs, const CompileResult &compileResult) {
        auto progSpecFileName = compileResult.outputDirPath + "/" + compileResult.progSpecFileName;
        auto sharedLibPath = compileResult.outputDirPath + "/" + compileResult.binFileName;

        ProgramSpec &progSpecObj = ProgramSpec::getInstance();
        if (!progSpecObj.initialize(progSpecFileName)) {
            throw std::runtime_error("Cannot initialize ProgramSpec");
        }

        FHERuntime rt(progSpecFileName);

        {
            auto result = rt.open(sharedLibPath);
            if (!result) {
                auto s = llvm::toString(result.takeError());
                throw std::runtime_error(s);
            }
        }

        {
            auto protoFunctions = progSpecObj.getFuncInfo();

            std::string funcName = "";
            for (auto func : protoFunctions) {
                funcName = func.asReader().getName();
                break;
            }
            if (funcName.empty()) {
                throw std::runtime_error("Cannot get function name");
            }
#ifndef NDEBUG
            std::cout << "funcName: " << funcName << std::endl;
#endif

            auto result = rt.resolveSymbol(funcName);
            if (!result) {
                auto s = llvm::toString(result.takeError());
                throw std::runtime_error(s);
            }
        }
        {
            // Serial various keys
            auto cryptoCtx = aegiscpu::openfhe::CryptoContextMgr::getInstance().getCryptoContext();

            const std::string ccFileName = compileResult.outputDirPath + "/__cryptocontext.bin";
            if (!Serial::SerializeToFile(ccFileName, cryptoCtx, SerType::BINARY)) {
                throw std::runtime_error("Error writing serialization of the crypto context");
            }

            const std::string pubKeyFileName = compileResult.outputDirPath + "/__pubkey.bin";
            std::shared_ptr<aegiscpu::openfhe::FHEPublicKey> aegisPubKey =
                aegiscpu::openfhe::FheKeyset::getInstance().getPubKey();
            PublicKey<DCRTPoly> pubKey = aegisPubKey->getKey();
            if (!Serial::SerializeToFile(pubKeyFileName, pubKey, SerType::BINARY)) {
                throw std::runtime_error("Error writing public keys");
            }

            const std::string mulKeyFileName = compileResult.outputDirPath + "/__mulkey.bin";
            std::ofstream multKeyFile(mulKeyFileName, std::ios::out | std::ios::binary);
            if (multKeyFile.is_open()) {
                if (!cryptoCtx->SerializeEvalMultKey(multKeyFile, SerType::BINARY)) {
                    throw std::runtime_error("Error writing eval mult keys");
                }
                multKeyFile.close();
            } else {
                throw std::runtime_error("Error serializing EvalMult keys");
            }

            std::string rotKeyFileName = "";
            auto keyInfo = progSpecObj.getKeyInfo();
            if (keyInfo.asBuilder().hasGaloisIndices()) {
                rotKeyFileName = compileResult.outputDirPath + "/__rotkey.bin";
                std::ofstream rotationKeyFile(rotKeyFileName, std::ios::out | std::ios::binary);
                if (rotationKeyFile.is_open()) {
                    if (!cryptoCtx->SerializeEvalAutomorphismKey(rotationKeyFile, SerType::BINARY)) {
                        throw std::runtime_error("Error writing rotation keys");
                    }
                    rotationKeyFile.close();
                } else {
                    throw std::runtime_error("Error serializing Rotation keys");
                }
            }

            auto result = rt.loadCryptoResources(ccFileName, pubKeyFileName, mulKeyFileName, rotKeyFileName);
            if (!result) {
                auto s = llvm::toString(result.takeError());
                throw std::runtime_error(s);
            }
        }

        std::vector<mlir::aegis::Value> inputs;
        for (auto pv : pvs) {
            inputs.insert(inputs.end(), pv.values.begin(), pv.values.end());
        }

        auto result = rt.call(inputs);
        if (!result) {
            auto s = llvm::toString(result.takeError());
            throw std::runtime_error(s);
        }

        std::vector<mlir::aegis::Value> res = *result;
        std::vector<PyValue> resPVs;
        for (auto &v : res) {
            PyValue pv;
            pv.ndim = 1; // origin
            pv.values.push_back(v);
            resPVs.push_back(pv);
        }

        return resPVs;
    }
};

/// @brief
class PySimRuntime {
  public:
    vector<PyValue> run(const vector<PyValue> &pvs, const CompileResult &compileResult) {
        auto simFileName = compileResult.outputDirPath + "/" + compileResult.simFileName;

        SimRuntime rt(simFileName);

        std::vector<mlir::aegis::Value> inputs;
        for (auto pv : pvs) {
            inputs.insert(inputs.end(), pv.values.begin(), pv.values.end());
        }

        auto result = rt.call(inputs);
        if (!result) {
            auto s = llvm::toString(result.takeError());
            throw std::runtime_error(s);
        }

        std::vector<mlir::aegis::Value> res = *result;
        std::vector<PyValue> resPVs;
        for (auto &v : res) {
            PyValue pv;
            pv.ndim = 1; // origin
            pv.values.push_back(v);
            resPVs.push_back(pv);
        }

        return resPVs;
    }
};

PYBIND11_MODULE(primus_aegis, m) {
    m.doc() = "Aegis";

    //
    //
    // Type
    py::class_<PyValue>(m, "Value")
        .def_static("from_bytes", [](const py::bytes &b) { return Utils::PyBytes2PyValue(b); })
        .def("to_bytes", [](PyValue &self) { return Utils::PyValue2PyBytes(self); });

    //
    //
    // Compiler
    py::module m_compiler = m.def_submodule("compiler");

    // Compiler Engine
    py::enum_<BACKEND_TYPE>(m_compiler, "BACKEND_TYPE")
        .value("CPU", BACKEND_TYPE::CPU)
        .value("GPU", BACKEND_TYPE::GPU)
        .export_values();
    py::enum_<TARGET>(m_compiler, "COMPILE_TARGET")
        .value("MLIR", TARGET::MLIR)
        .value("LOWER_MLIR", TARGET::LOWER_MLIR)
        .value("SECRET", TARGET::SECRET)
        .value("FHE", TARGET::FHE)
        .value("EMITC", TARGET::EMITC)
        .value("CPP", TARGET::CPP)
        .value("LIBRARY", TARGET::LIBRARY)
        .value("SIM_MLIR", TARGET::SIM_MLIR)
        .export_values();
    py::enum_<FHE_SCHEME_TYPE>(m_compiler, "FHE_SCHEME_TYPE")
        .value("BGV", FHE_SCHEME_TYPE::BGV)
        .value("BFV", FHE_SCHEME_TYPE::BFV)
        .value("CKKS", FHE_SCHEME_TYPE::CKKS)
        .export_values();

    py::class_<CompileOptions>(m_compiler, "CompileOption")
        .def(py::init<>())
        .def_readwrite("backendType", &CompileOptions::beType)
        .def_readwrite("compileTarget", &CompileOptions::target)
        .def_readwrite("scheme", &CompileOptions::scheme)
        .def_readwrite("verbose", &CompileOptions::verbose)
        .def_readwrite("outputDir", &CompileOptions::outputDir)
        .def(
            "to_json", [](const CompileOptions &x, const int indent = -1) { return nlohmann::json(x).dump(indent); },
            py::arg("indent") = -1)
        .def_static("from_json", [](const std::string &s) { return nlohmann::json::parse(s).get<CompileOptions>(); });
    py::class_<CompileResult>(m_compiler, "CompileResult")
        .def(py::init<>())
        .def_readwrite("outputDirPath", &CompileResult::outputDirPath)
        .def_readwrite("cppFileName", &CompileResult::cppFileName)
        .def_readwrite("simFileName", &CompileResult::simFileName)
        .def_readwrite("binFileName", &CompileResult::binFileName)
        .def_readwrite("progSpecFileName", &CompileResult::progSpecFileName)
        .def(
            "to_json", [](const CompileResult &x, const int indent = -1) { return nlohmann::json(x).dump(indent); },
            py::arg("indent") = -1)
        .def_static("from_json", [](const std::string &s) { return nlohmann::json::parse(s).get<CompileResult>(); });

    py::class_<PyCompiler>(m_compiler, "Compiler")
        .def(py::init<>())
        .def("compile", &PyCompiler::compile, py::arg("mlir_content"), py::arg("compile_options"));

    //
    //
    // FHE
    py::module m_fhe = m.def_submodule("fhe");

    // FHE Key
    py::class_<KeyInfo>(m_fhe, "KeyInfo")
        .def(py::init<>())
        .def_readwrite("polyModDegree", &KeyInfo::polyModDegree, "poly modulus degree.")
        .def_readwrite("coffModCh", &KeyInfo::coffModCh, "coff modulus chain.")
        .def_readwrite("scale", &KeyInfo::scale, "scale factor.")
        .def_readwrite("multDepth", &KeyInfo::multDepth, "multiplication depth")
        .def_readwrite("firstModSize", &KeyInfo::firstModSize, "first modulus size")
        .def_readwrite("scaleModSize", &KeyInfo::scaleModSize, "scale modulus size")
        .def_readwrite("batchSize", &KeyInfo::batchSize, "batch size (number of slots)")
        .def_readwrite("galoisIndices", &KeyInfo::galoisIndices, "index list for Galois Key")
        .def_readwrite("enableBootstrapping", &KeyInfo::enableBootstrapping, "whether to enable bootstrapping")
        .def_readwrite("scheme", &KeyInfo::scheme, "FHE scheme type(ckks,bfv,bgv)");

    py::class_<FHEPrivateKey, std::shared_ptr<FHEPrivateKey>>(m_fhe, "PrivateKey")
        .def(py::init<>())
        .def("from_bytes", [](FHEPrivateKey &self, py::bytes b) { self.deserialize(b); })
        .def("to_bytes", [](FHEPrivateKey &self) { return py::bytes(self.serialize()); });
    py::class_<FHEPublicKey, std::shared_ptr<FHEPublicKey>>(m_fhe, "PublicKey")
        .def(py::init<>())
        .def("from_bytes", [](FHEPublicKey &self, py::bytes b) { self.deserialize(b); })
        .def("to_bytes", [](FHEPublicKey &self) { return py::bytes(self.serialize()); });

    py::class_<FheKeyset>(m_fhe, "Keyset")
        .def_static("getInstance", &FheKeyset::getInstance, py::return_value_policy::reference)
        .def("generate", &generate_keyset, py::arg("prog_spec_file"))
        .def("from_bytes", [](FheKeyset &self, py::bytes b) { self.loads(b); })
        .def(
            "to_bytes", [](FheKeyset &self, bool c) { return py::bytes(self.dumps(c)); }, py::arg("contain_sk") = true)
        .def("getPrivateKey", &FheKeyset::getPriKey, py::return_value_policy::automatic)
        .def("getPublicKey", &FheKeyset::getPubKey, py::return_value_policy::automatic);

    //
    //
    // DataProcessor
    py::module m_dp = m.def_submodule("dataprocessor");

    // FHE DataProcessor
    py::class_<PyDataProcessor<>>(m_dp, "FHEDataProcessor")
        .def_static("privateInput", &PyDataProcessor<>::privateInput)
        .def_static("publicInput", &PyDataProcessor<>::publicInput)
        .def_static("processOutput", &PyDataProcessor<>::processOutput);

    // Sim DataProcessor
    py::class_<PyDataProcessor<SimDataProcessor, true>>(m_dp, "SimDataProcessor")
        .def_static("privateInput", &PyDataProcessor<SimDataProcessor, true>::privateInput)
        .def_static("publicInput", &PyDataProcessor<SimDataProcessor, true>::publicInput)
        .def_static("processOutput", &PyDataProcessor<SimDataProcessor, true>::processOutput);

    //
    //
    // Runtime
    py::module m_rt = m.def_submodule("runtime");

    // FHE Runtime
    py::class_<PyFHERuntime>(m_rt, "FHERuntime")
        .def(py::init<>())
        .def("run", &PyFHERuntime::run, py::arg("input"), py::arg("compile_result"));

    // Sim Runtime
    py::class_<PySimRuntime>(m_rt, "SimRuntime")
        .def(py::init<>())
        .def("run", &PySimRuntime::run, py::arg("input"), py::arg("compile_result"));
}
