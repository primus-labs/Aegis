#ifndef COMMON_PROGRAMSPEC_H
#define COMMON_PROGRAMSPEC_H

#include "Protocol.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <mutex>

namespace mlir {
namespace aegis {

class ProgramSpec {
private:
  ProtoMessage<aegisprotocol::ProgSpec> progSpec;
  ProtoMessage<aegisprotocol::KeyInfo> keyInfo;
  std::vector<ProtoMessage<aegisprotocol::Function>> funcsInfo;
  ProtoMessage<aegisprotocol::StatsInfo> statsInfo;

private:
  ProgramSpec() = default;
  ProgramSpec(const ProgramSpec &) = delete;
  ProgramSpec &operator=(const ProgramSpec &) = delete;

public:
  static ProgramSpec &getInstance() {
    static ProgramSpec instance;
    return instance;
  }

  bool initialize(const std::string &progSpecFile) {
    if (progSpecFile.empty()) {
      return false;
    }
    static std::once_flag flag;
    static bool initSuccess = true;

    std::call_once(flag, [&]() {
      std::ifstream specFile(progSpecFile);
      std::string content((std::istreambuf_iterator<char>(specFile)),
                          (std::istreambuf_iterator<char>()));

      if (specFile.fail()) {
        ErrorMsg err;
        err << "Cannot read program spec info file...";
        initSuccess = false;
        return;
      }

      progSpec.readJsonFromString(content);
      keyInfo = progSpec.asReader().getKeyInfo();
      statsInfo = progSpec.asReader().getStatsInfo();
      auto funcs = progSpec.asReader().getFuncsInfo();
      for (auto func : funcs.getFunctions()) {
        funcsInfo.push_back((ProtoMessage<aegisprotocol::Function>)func);
      }
    });

    return initSuccess;
  }

public:
  ProtoMessage<aegisprotocol::ProgSpec> getProgSpec() const { 
      return progSpec; 
  }

  ProtoMessage<aegisprotocol::KeyInfo> getKeyInfo() const { 
      return keyInfo; }

  std::vector<ProtoMessage<aegisprotocol::Function>> getFuncInfo() const {
      return funcsInfo;
  }

  ProtoMessage<aegisprotocol::StatsInfo> getStatsInfo() const {
      return statsInfo;
  }
};

} // namespace aegis
} // namespace mlir

#endif
