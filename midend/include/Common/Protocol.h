#ifndef PROTOCOL_ERROR_H
#define PROTOCOL_ERROR_H

#include "capnp/common.h"
#include "capnp/compat/json.h"
#include "capnp/message.h"
#include "capnp/serialize-packed.h"
#include "capnp/serialize.h"
#include "protocol.capnp.h"
#include "kj/common.h"
#include "kj/exception.h"
#include "kj/io.h"
#include "kj/std/iostream.h"
#include "kj/string.h"
#include "Common/Error.h"
#include <cstddef>
#include <memory>
#include <optional>
#include <sstream>
#include <vector>
#include <fstream>
#include <algorithm>


using mlir::aegis::ErrorMsg;

namespace mlir {
namespace aegis {

template <typename MsgType> 
struct ProtoMessage {
private:
    capnp::MallocMessageBuilder *msgBuilder;
    typename MsgType::Builder msg;

public:
    /// Construtor
    ProtoMessage() : msg(nullptr) {
        msgBuilder = new capnp::MallocMessageBuilder();
        assert(msgBuilder);
        msg = msgBuilder->initRoot<MsgType>();
    }

    explicit ProtoMessage(const typename MsgType::Reader &reader) : msg(nullptr) {
        msgBuilder = new capnp::MallocMessageBuilder(
                                std::min(reader.totalSize().wordCount, capnp::MAX_SEGMENT_WORDS),
                                capnp::AllocationStrategy::FIXED_SIZE);
        assert(msgBuilder);
        msgBuilder->setRoot(reader);
        msg = msgBuilder->getRoot<MsgType>();
    }

    ProtoMessage(const ProtoMessage &input) : msg(nullptr) {
        msgBuilder = new capnp::MallocMessageBuilder(
                                std::min(input.msg.asReader().totalSize().wordCount,
                                capnp::MAX_SEGMENT_WORDS), capnp::AllocationStrategy::FIXED_SIZE);
        assert(msgBuilder);
        msgBuilder->setRoot(input.msg.asReader());
        msg = msgBuilder->getRoot<MsgType>();
    }

    ProtoMessage(ProtoMessage &&input) : msg(nullptr) {
        msgBuilder = input.msgBuilder;
        msg = input.msg;
        input.msgBuilder = nullptr;
    }

    /// Operator overload function
    ProtoMessage &operator=(const typename MsgType::Reader &reader) {
        if (msgBuilder)
            delete msgBuilder;
        
        msgBuilder = new capnp::MallocMessageBuilder(
                                std::min(reader.totalSize().wordCount, capnp::MAX_SEGMENT_WORDS),
                                capnp::AllocationStrategy::FIXED_SIZE);
        assert(msgBuilder);
        msgBuilder->setRoot(reader);
        msg = msgBuilder->getRoot<MsgType>();
        return *this;
    }

    ProtoMessage &operator=(const ProtoMessage &input) {
        if (this != &input) {
            if (msgBuilder) {
                delete msgBuilder;
            }

            msgBuilder = new capnp::MallocMessageBuilder(
                                    std::min(input.msg.asReader().totalSize().wordCount,
                                    capnp::MAX_SEGMENT_WORDS), capnp::AllocationStrategy::FIXED_SIZE);
            assert(msgBuilder);
            msgBuilder->setRoot(input.msg.asReader());
            msg = msgBuilder->getRoot<MsgType>();
        }

        return *this;
    }

    ProtoMessage &operator=(ProtoMessage &&input) {
        if (this != &input) {
            if (msgBuilder) {
                delete msgBuilder;
            }

            msgBuilder = input.msgBuilder;
            msg = input.msg;
            input.msgBuilder = nullptr;
        }
        return *this;
    }

    bool operator==(ProtoMessage const &other) const {
        capnp::AnyStruct::Reader lhs = this->asReader();
        capnp::AnyStruct::Reader rhs = other.asReader();
        return lhs == rhs;
    }

    bool operator!=(ProtoMessage const &other) const {
        capnp::AnyStruct::Reader lhs = this->asReader();
        capnp::AnyStruct::Reader rhs = other.asReader();
        return lhs != rhs;
    }

    /// Destructor
    ~ProtoMessage() {
        if (msgBuilder) {
            delete msgBuilder;
        }
    }

    typename MsgType::Reader asReader() const { return msg.asReader(); }
    typename MsgType::Builder asBuilder() { return msg; }


    /// Serialization-related Functions
    void writeBinaryToOstream(std::ostream &os) const {
        try {
            kj::std::StdOutputStream kjOstream(os);
            capnp::writeMessage(kjOstream, *msgBuilder);
        } catch (const kj::Exception &e) {
            ErrorMsg err;
            err << "Failed to write message to ostream: " << e.getDescription().cStr();
            std::abort();
        } catch (...) {
            ErrorMsg err;
            err << "Failed to write message to ostream.";
            std::abort();
        }
        os.flush();
        if (!os.good()) {
            ErrorMsg err;
            err << "Failed to write message to ostream, ended up in bad state.";
            std::abort();
        }
    }

    void writeBinaryToString() const {
        auto ostream = std::ostringstream();
        this->writeBinaryToOstream(ostream);
    }

    std::string writeJsonToString() const {
        try {
            capnp::JsonCodec json;
            kj::String output = json.encode(this->msg.asReader());
            return std::string(output.cStr(), output.size());
        } catch (const kj::Exception &e) {
            ErrorMsg err;
            err << "Failed to write message to json string: " << e.getDescription().cStr();
            std::abort();
        } catch (...) {
            ErrorMsg err;
            err << "Failed to write message to json string.";
            std::abort();
        }
    }

    void readBinaryFromIstream(std::istream &istream,
                        capnp::ReaderOptions options = capnp::ReaderOptions()) {
        try {
            kj::std::StdInputStream kjIstream(istream);
            capnp::readMessageCopy(kjIstream, *msgBuilder, options);
            this->msg = msgBuilder->getRoot<MsgType>();
        } catch (const kj::Exception &e) {
            ErrorMsg err;
            err << "Failed to read message from istream: " << e.getDescription().cStr();
            std::abort();
        } catch (...) {
            ErrorMsg err;
            err << "Failed to read message from istream.";
            std::abort();
        }
    }

    void readBinaryFromString(const std::string &input,
                        capnp::ReaderOptions options = capnp::ReaderOptions()) {
        auto istream = std::istringstream(input);
        this->readBinaryFromIstream(istream, options);
    }

    void readJsonFromString(const std::string &input) {
        try {
            capnp::JsonCodec json;
            kj::StringPtr stringPointer(input.c_str(), input.size());
            this->msg = this->msgBuilder->template initRoot<MsgType>();
            json.decode(stringPointer, this->msg);
        } catch (const kj::Exception &e) {
            ErrorMsg err;
            err << "Failed to read message from json string: " << e.getDescription().cStr();
            std::abort();
        } catch (...) {
            ErrorMsg err;
            err << "Failed to read message from json string.";
            std::abort();
        }
    }

    std::string debugString() const { 
        return writeJsonToString().value(); 
    }
};

} // namespace aegis 
} // namespace mlir

#endif