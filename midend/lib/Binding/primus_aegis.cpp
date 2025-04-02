
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;

#include "Common/Protocol.h"
#include "Common/Value.h"
#include "Runtime/FHE/FHEDataProcessor.h"
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <capnp/serialize.h>
#include <protocol.capnp.h>
using namespace mlir::aegis;

#include "FheKey.h"
#include "KeysetGenerator.h"
#include "Operate.h"
using namespace aegiscpu;

#include <iostream>
#include <sstream>
using namespace std;

#define DEBUG_PRINT 1

struct KeyInfo {
    uint32_t polyModDegree;
    std::vector<uint32_t> coffModCh;
    uint32_t scale;
    uint32_t multDepth;
    uint32_t scaleModSize;
    uint32_t batchSize;
    std::vector<int32_t> galoisIndices;
    bool enableBootstrapping;
    uint32_t numSlot;
};

static void makeProtoKeyInfo(const KeyInfo &keyInfo, ProtoMessage<aegisprotocol::KeyInfo> &protoKeyInfo) {
    protoKeyInfo.asBuilder().setPolyModDegree(keyInfo.polyModDegree);
    protoKeyInfo.asBuilder().setScale(keyInfo.scale);
    protoKeyInfo.asBuilder().setMultDepth(keyInfo.multDepth);
    protoKeyInfo.asBuilder().setScaleModSize(keyInfo.scaleModSize);
    protoKeyInfo.asBuilder().setBatchSize(keyInfo.batchSize);
    protoKeyInfo.asBuilder().setEnableBootstrapping(keyInfo.enableBootstrapping);
    protoKeyInfo.asBuilder().setNumSlot(keyInfo.numSlot);

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

static void generate_keyset(FheKeyset &self, const KeyInfo &keyInfo) {
    // TODO: Should we need call once here?
    static std::once_flag initFlag;
    std::call_once(initFlag, [&]() {
        ProtoMessage<aegisprotocol::KeyInfo> protoKeyInfo;
        makeProtoKeyInfo(keyInfo, protoKeyInfo);

        // initialize
        KeysetGenerator::generate(protoKeyInfo);
    });
}

class Utils {
  public:
    /**
     * Convert numpy.array(o, dtype=np.float64) to Value(double)
     */
    static Value Numpy2Value(const py::array_t<double> &input) {
        auto buf = input.request();
#if DEBUG_PRINT
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
        cout << "dims.size() " << dims.size() << endl;
        cout << "values " << values.size() << endl;
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
};

/// @brief
class PyFHEDataProcessor {
  public:
    static py::bytes privateInput(const py::array_t<double> &input) {
        auto plainValue = Utils::Numpy2Value(input);
        std::vector<Value> plainValues = {plainValue}; // TODO: no need vector<Value> for DataProcessor.privateInput
        auto cipherValues = FHEDataProcessor().privateInput(plainValues);
        return Utils::Value2PyBytes(cipherValues[0]);
    }
    static py::array_t<double> processOutput(const py::bytes &input) {
        auto cipherValue = Utils::PyBytes2Value(input);
        std::vector<Value> cipherValues = {cipherValue}; // TODO: no need vector<Value> for DataProcessor.processOutput
        std::vector<size_t> pp = {8};
        auto plainValues = FHEDataProcessor().processOutput(cipherValues, pp);
        return Utils::Value2Numpy(plainValues[0]);
    }
};

PYBIND11_MODULE(primus_aegis, m) {
    m.doc() = "Aegis";

    // FHE
    py::module m_fhe = m.def_submodule("fhe");

    // FHE Key
    py::class_<KeyInfo>(m_fhe, "KeyInfo")
        .def(py::init<>())
        .def_readwrite("polyModDegree", &KeyInfo::polyModDegree, "poly modulus degree.")
        .def_readwrite("coffModCh", &KeyInfo::coffModCh, "coff modulus chain.")
        .def_readwrite("scale", &KeyInfo::scale, "scale factor.")
        .def_readwrite("multDepth", &KeyInfo::multDepth, "multiplication depth")
        .def_readwrite("scaleModSize", &KeyInfo::scaleModSize, "scale modulus size")
        .def_readwrite("batchSize", &KeyInfo::batchSize, "batch size")
        .def_readwrite("galoisIndices", &KeyInfo::galoisIndices, "index list for Galois Key")
        .def_readwrite("enableBootstrapping", &KeyInfo::enableBootstrapping, "whether to enable bootstrapping")
        .def_readwrite("numSlot", &KeyInfo::numSlot, "number of slots");

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
        .def("generate", &generate_keyset, py::arg("key_info"))
        .def("from_bytes", [](FheKeyset &self, py::bytes b) { self.loads(b); })
        .def(
            "to_bytes", [](FheKeyset &self, bool c) { return py::bytes(self.dumps(c)); }, py::arg("contain_sk") = true)
        .def("getPrivateKey", &FheKeyset::getPriKey, py::return_value_policy::automatic)
        .def("getPublicKey", &FheKeyset::getPubKey, py::return_value_policy::automatic);

    // FHE DataProcessor
    py::class_<PyFHEDataProcessor>(m_fhe, "DataProcessor")
        .def_static("privateInput", &PyFHEDataProcessor::privateInput)
        .def_static("processOutput", &PyFHEDataProcessor::processOutput);
}
