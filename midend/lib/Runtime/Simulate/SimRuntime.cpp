#include "Runtime/Simulate/SimRuntime.h"
#include "Common/DoubleSerializer.h"
#include "Common/Error.h"
#include "Common/Utils.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/FormatVariadic.h"
#include <fstream>
#include <iostream>
// #include <regex>

#ifdef __APPLE__
    #define DYLIB_EXT ".dylib"
#else // linux, etc.
    #define DYLIB_EXT ".so"
#endif

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
    outputFile.close();

    return true;
}

llvm::Expected<std::vector<Value>> SimRuntime::call(const std::vector<Value> &input) {
    // Get argument values and convert to strings
    std::vector<std::string> argsStrContent;
    for (auto & val : input) {
        std::string unitArgContent;
        auto dims = val.getDims();
        auto tensors = val.getTensor<uint8_t>().value().values;
        if (dims.size() == 1) {
            if (dims[0] == 0) {
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
                    unitArgContent += std::to_string(itemRealVals[j]);
                    if (j != (itemRealVals.size() - 1)) {
                        unitArgContent += ',';
                    }
                }
                unitArgContent += ']';

                if (i != (realVals.size() - 1)) {
                    unitArgContent += ',';
                }
            }
            unitArgContent += ']';
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
    // Find aegiscompiler tool path
    std::string aegisCompileTool = aegis::findAegisTool("aegiscompiler", "AEGIS_COMPILER_PATH");
    if (aegisCompileTool.empty()) {
        return ErrorMsg("aegisCompiler not found in PATH or AEGIS_COMPILER_PATH.\n");
    }
    std::vector<std::string> args = {
        "--one-shot-bufferize",
        "--convert-linalg-to-loops",
        "--convert-scf-to-cf",
        "--expand-strided-metadata",
        "--lower-affine",
        "--convert-arith-to-llvm",
        "--convert-scf-to-cf",
        "--finalize-memref-to-llvm",
        "--convert-func-to-llvm",
        "--convert-cf-to-llvm",
        "--reconcile-unrealized-casts",
        simMlirFileName,
    };
    std::string llvmLevelMlirContent, errContent;
    if (executeAegisTool(aegisCompileTool, args, llvmLevelMlirContent, errContent)) {
        return ErrorMsg(errContent);
    }

    // Write lower lower mlir content to the mlri file
    std::ofstream outFile(simMlirFileName);
    if (!outFile) {
        return ErrorMsg("Failed to write file: ") << simMlirFileName;
    }
    outFile << llvmLevelMlirContent;
    outFile.close();

    // Exec mlir_cpu_runner tool to run llvm dialect mlir to get result value.
    std::string mlirRunTool = aegis::findAegisTool("mlir-cpu-runner", "MLIR_RUNNER_PATH");
    if (mlirRunTool.empty()) {
        return ErrorMsg("mlir-cpu-runner not found in PATH or MLIR_RUNNER_PATH.\n");
    }
    auto parentPath = llvm::sys::path::parent_path(llvm::StringRef(mlirRunTool));
    parentPath = llvm::sys::path::parent_path(llvm::StringRef(parentPath));
    std::string argShareLib = std::string(llvm::formatv("-shared-libs={0}/lib/libmlir_runner_utils{1}", 
                                          parentPath.str(), DYLIB_EXT));
    std::vector<std::string> argsRun = {
        "-e",
        "main",
        "-O2",
        "-entry-point-result=void",
        argShareLib,
        simMlirFileName,
    };
    std::string runMlirRetContent;
    if (executeAegisTool(mlirRunTool, argsRun, runMlirRetContent, errContent)) {
        return ErrorMsg(errContent);
    }

    // Handling the return value
    // Parse the return value, extract the final value, and assemble it into a Value object for return
    auto retAryorError = parseMlirRunnerOutput(runMlirRetContent);
    if (!retAryorError) {
        return retAryorError.takeError();
    }
    auto retAry = *retAryorError;
    auto outSizes = retAry.size();
    auto inSizes = retAry[0].size();
    if (outSizes == 1) {
        if (inSizes == 1) {
            auto bytes = serializeFormDouble(retAry[0][0]);
            return std::vector<Value>{Value(Tensor<uint8_t>(bytes, std::vector<size_t>{inSizes}))};
            //std::vector<double> res{retAry[0][0]};
            //return std::vector<Value>{Value(Tensor<double>(res, std::vector<size_t>{inSizes}))};
        } else {
            auto bytes = serializeFormVectorDouble(retAry[0]);
            return std::vector<Value>{Value(Tensor<uint8_t>(bytes, std::vector<size_t>{inSizes}))};
            //std::vector<double> res = retAry[0];
            //return std::vector<Value>{Value(Tensor<double>(res, std::vector<size_t>{inSizes}))};
        }
    } else {
        std::vector<double> res;
        for (auto i = 0; i < retAry.size(); i++) {
            res.insert(res.end(), retAry[i].begin(), retAry[i].end());
        }
        auto bytes = serializeFormVectorDouble(res);
        return std::vector<Value>{Value(Tensor<uint8_t>(bytes, std::vector<size_t>{outSizes, inSizes}))};
        //return std::vector<Value>{Value(Tensor<double>(res, std::vector<size_t>{outSizes, inSizes}))};
    }
}

llvm::Expected<std::vector<std::vector<double>>> SimRuntime::parseMlirRunnerOutput(const std::string& output) {
    // Locate the starting position of data section
    const std::string marker = "data =";
    auto startPos = output.find(marker);
    if (startPos == std::string::npos) {
        return ErrorMsg("Data section marker not found");
    }
    startPos += marker.length();
    
    // Find the starting bracket '[' for the array
    auto arrayStart = output.find('[', startPos);
    if (arrayStart == std::string::npos) {
        return ErrorMsg("Array start marker not found");
    }
    
    // Initialize parsing states and storage variables
    std::vector<std::vector<double>> result;
    std::vector<double> curRow;
    std::string curNum;     
    int bracketLevel = 1;        // Track nested array levels (1 = outer, 2 = inner)
    
    // Start parsing from array beginning
    for (size_t i = arrayStart + 1; i < output.size() && bracketLevel > 0; ++i) {
        char c = output[i];
        
        if (bracketLevel == 1) { 
            if (c == '[') {
                // Start a new row (2D array)
                bracketLevel = 2;
                curRow.clear();
                curNum.clear(); 
            } else if (c == ']') { 
                // End of outer array
                bracketLevel--;
            } else if (std::isdigit(c) || c == '.' || c == '-' || c == 'e' || c == 'E') {
                // Handle numbers or scientific notation directly in 1D array
                bracketLevel = 2;           
                curRow.clear();
                curNum.clear();
                i--;
            }
            // Ignore other characters (commas/spaces)
        } else if (bracketLevel == 2) {
            if (std::isdigit(c) || c == '.' || c == '-' || c == 'e' || c == 'E') {
                // Accumulate number characters
                curNum += c;
            } else if (c == ',' || c == ']') {
                // End of number or row
                if (!curNum.empty()) {
                    try {
                        curRow.push_back(std::stod(curNum));
                    } catch (...) {
                        return ErrorMsg("Invalid number format: ") << curNum;
                    }
                    curNum.clear();
                }
                
                if (c == ']') {
                    // End of current row
                    bracketLevel--;
                    result.push_back(curRow); // Save even if empty
                    curRow.clear();           // Reset for next row

                    // Check if next non-whitespace character ends the entire array
                    size_t nextIndex = i + 1;
                    // Skip whitespace
                    while (nextIndex < output.size() && 
                          std::isspace(static_cast<unsigned char>(output[nextIndex]))) {
                        nextIndex++;
                    }
                    
                    if (nextIndex >= output.size()) {
                        // String ends after row: close entire array
                        bracketLevel--;
                        break;  
                    } else if (output[nextIndex] == ']') {
                        // Subsequent ']' closes entire array
                        bracketLevel--;
                        i = nextIndex;
                        break;
                    }
                }
            } else if (c == '[') {
                return ErrorMsg("Nested arrays are not supported");
            }
            // Ignore other characters (spaces/commas inside rows)
        }
    }
    
    // Verify all brackets are closed
    if (bracketLevel != 0) {
        return ErrorMsg("Array brackets not properly closed");
    }
    
    return result;
}

} // namespace aegis
} // namespace mlir