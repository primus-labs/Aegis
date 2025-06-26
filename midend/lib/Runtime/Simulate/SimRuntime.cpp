#include "Runtime/Simulate/SimRuntime.h"
#include "Common/DoubleSerializer.h"
#include "Common/Error.h"
#include <fstream>
#include <iostream>
// #include <regex>


namespace mlir {
namespace aegis {

llvm::Expected<bool> SimRuntime::open(const std::string &sharedLibPath) {
    return ErrorMsg("open not supported in SimRuntime");
}

llvm::Expected<bool> SimRuntime::loadCryptoResources(const std::string &cryptCtxFileName,
                                            const std::string &pubKeyFileName, 
                                            const std::string &multKeyFileName, 
                                            const std::string &rotKeyFileName) {
    return ErrorMsg("loadCryptoResources not supported in SimRuntime");
}

llvm::Expected<bool> SimRuntime::resolveSymbol(const std::string &funcName) {
    return ErrorMsg("resolveSymbol not supported in SimRuntime");
}

llvm::Expected<std::vector<Value>> SimRuntime::call(const std::vector<Value> &input) {
    return ErrorMsg("call not supported in SimRuntime");
}

llvm::Expected<bool> SimRuntime::replaceMlirTokenWith(const std::vector<std::string> &replacements) {
    // Read file content to a string
    std::ifstream inputFile(simMlirFileName);
    if (!inputFile) {
        return ErrorMsg("Failed to open file: ") << simMlirFileName;
    }
    std::string content = std::string(std::istreambuf_iterator<char>(inputFile), std::istreambuf_iterator<char>());
    inputFile.close();

    /*
    // Perform token replacement using regex
    // Regex pattern: matches $number$ format
    std::regex pattern(R"(\$(\d+)\$)");
    
    // Regex replacement using callback function
    content = std::regex_replace(content, pattern, [&](const std::smatch& match) -> std::string {
            // Extract matched number
            std::string numStr = match[1].str();
            
            try {
                // Convert string to index
                size_t index = static_cast<size_t>(std::stoul(numStr));
                
                // Validate index is within replacement vector size
                if (index < replacements.size()) {
                    return replacements[index]; // Return replacement string
                }
            } catch (const std::exception& e) {
                // Ignore if number conversion fails
            }
            
            // Keep original token for invalid indices or conversion failures
            return match[0].str();
        }
    );
    */

    // Perform token replacement
    size_t pos = 0;
    while ((pos = content.find('$', pos)) != std::string::npos) {
        // Ensure enough characters for a complete token ($X$)
        if (pos + 2 >= content.size()) {
            break;  // Incomplete token at end of file
        }

        // Check if third character is closing $
        if (content[pos + 2] != '$') {
            pos++;  //Skip invalid token
            continue;
        }

        // Get character inside token
        char numChar = content[pos + 1];
        
        // Validate if it's a digit
        if (!std::isdigit(static_cast<unsigned char>(numChar))) {
            pos += 2;  // Skip $ and invalid char
            continue;
        }
        
        // Convert to index value
        size_t index = static_cast<size_t>(numChar - '0');
        
        // Validate index is within replacement vector size
        if (index >= replacements.size()) {
            pos += 3;  //Skip entire invalid token
            continue;
        }

        // Perform replacement
        content.replace(pos, 3, replacements[index]);
        
        // Move position pointer after replacement
        pos += replacements[index].size();
    }

    // Write modified content back to file
    std::ofstream outputFile(simMlirFileName);
    if (!outputFile) {
        return ErrorMsg("Failed to write file: ") << simMlirFileName;
    }
    
    outputFile << content;
    return true;
}

llvm::Expected<std::vector<Value>> SimRuntime::simulate(const std::vector<Value> &input) {
    // Get argument values and convert to strings
    std::vector<std::string> argsStrContent;
    for (auto & val : input) {
        std::string unitArgContent;
        auto dims = val.getDims();
        auto tensors = val.getTensor<uint8_t>().value().values;
        if (dims.size() == 1) {
            if (dims[0] == 1) {
                auto realVal = aegis::deserializeToDouble(tensors);
                unitArgContent = std::to_string(realVal);
            } else {
                // fill unitArgContent like: [1.1, 2.2], [3.3, 3.4] 
                std::vector<double> realVals = aegis::deserializeToVectorDouble(tensors);
                assert(realVals.size() == dims[0] && "The actual argument value does not match its dimension.");

                unitArgContent = '[';
                for (auto i = 0; i < realVals.size(); i++) {
                    unitArgContent += std::to_string(realVals[i]);
                    if (i != (realVals.size() - 1)) {
                        unitArgContent += ',';
                    }
                }
                unitArgContent += ']';
            }
        } else if (dims.size() == 2) {
            // fill unitArgContent like: [ [1.1, 2.2], [3.3, 3.4]] 
            std::vector<std::vector<double>> realVals = aegis::deserializeToMatrixDouble(tensors);
            assert((realVals.size() == dims[0] && realVals[0].size() == dims[1]) && 
                    "The actual argument value does not match its dimension.");

            unitArgContent = '[';
            for (auto i = 0; i < realVals.size(); i++) {
                auto itemRealVals = realVals[i];

                unitArgContent += '[';
                for (auto j = 0; j < itemRealVals.size(); j++) {
                    unitArgContent += std::to_string(itemRealVals[i]);
                    if (j != (itemRealVals.size() - 1)) {
                        unitArgContent += ',';
                    }
                }
                unitArgContent += ']';

                if (i != (realVals.size() - 1)) {
                    unitArgContent += ',';
                }
            }
        } else {
            assert(false && "Dimensions higher than 2D are currently not supported.");
        }

        argsStrContent.emplace_back(unitArgContent);
    }

    // Replaces tokens (e.g. $0$) in the mlir file with specified strings(argsStrContent)
    auto resReplace = replaceMlirTokenWith(argsStrContent);
    if (!resReplace) {
        return resReplace.takeError();
    }

    // Exec aegiscompile tool to low level mlir file to llvm dialect mlir.

    // Exec mlir_cpu_runner tool to run llvm dialect mlir to get result value.


    return input;
}

} // namespace aegis
} // namespace mlir