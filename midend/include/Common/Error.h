#ifndef COMMON_ERROR_H
#define COMMON_ERROR_H

#include <llvm/Support/Error.h>


namespace mlir {
namespace aegis {

class ErrorMsg {

public:
    ErrorMsg(const llvm::StringRef &s) : msg(s.str()), os(msg) {};
    ErrorMsg() : msg(""), os(msg) {};

    template <typename T> 
    ErrorMsg &operator<<(const T &v) {
        this->os << v;
        return *this;
    }

    operator llvm::Error() {
        return llvm::make_error<llvm::StringError>(os.str(), llvm::inconvertibleErrorCode());
    }

    template <typename T> 
    operator llvm::Expected<T>() {
        return this->operator llvm::Error();
    }

protected:
    std::string msg;
    llvm::raw_string_ostream os;
};



inline ErrorMsg &operator<<(ErrorMsg &errmsg, llvm::Error &err) {
    errmsg << llvm::toString(std::move(err));
    return errmsg;
}

} // namespace aegis
} // namespace mlir


#endif