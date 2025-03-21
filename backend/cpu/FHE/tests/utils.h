#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
using namespace std;

std::vector<double> RandomVector(size_t size, double minValue,
                                 double maxValue) {
  std::vector<double> vec(size);
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dist(minValue, maxValue);

  for (double &num : vec) {
    num = dist(gen);
  }
  return vec;
}

void PrintDiff(const std::vector<double> &v1, const std::vector<double> &v2) {
  if (v1.size() != v2.size()) {
    cout << "v1.size() != v2.size(): " << v1.size() << " " << v2.size() << endl;
    return;
  }

  for (size_t i = 0; i < v1.size(); ++i) {
    cout << v1[i] << " - " << v2[i] << " = " << v1[i] - v2[i] << endl;
  }
}

bool IsEqual(const std::vector<double> &v1, const std::vector<double> &v2,
             double epsilon = 1e-9) {
  if (v1.size() != v2.size()) {
    cout << "v1.size() != v2.size(): " << v1.size() << " " << v2.size() << endl;
    return false;
  }

  for (size_t i = 0; i < v1.size(); ++i) {
    if (std::fabs(v1[i] - v2[i]) > epsilon) {
      return false;
    }
  }
  return true;
}

#define assert_eq(a, b, c)                                                     \
  if (!IsEqual(a, b)) {                                                        \
    cout << "Not Equal:" << #c << endl;                                        \
  }

void toFile(const std::string &filepath, const std::string &data) {
  std::ofstream outFile(filepath, std::ios::out | std::ios::binary);
  if (!outFile) {
    std::cerr << "Error opening file for writing.\n";
    return;
  }
  outFile << data;
  outFile.close();
}

void fromFile(const std::string &filepath, std::string &data) {
  std::ifstream inFile(filepath, std::ios::in | std::ios::binary);
  if (!inFile) {
    std::cerr << "Error opening file for reading.\n";
    return;
  }
  // Read file contents into a string
  data.assign((std::istreambuf_iterator<char>(inFile)),
              std::istreambuf_iterator<char>());
  inFile.close();
}

const static std::string TEMPFOLDER = "./temp/";
// Macros for serialize/deserialize
#define DumpToFile(path, obj)                                                  \
  if (!Serial::SerializeToFile(TEMPFOLDER + path, obj, SerType::BINARY)) {     \
    std::cerr << "Error writing " << #obj << " to " << TEMPFOLDER + path       \
              << std::endl;                                                    \
  }

#define LoadFromFile(path, obj)                                                \
  if (!Serial::DeserializeFromFile(TEMPFOLDER + path, obj, SerType::BINARY)) { \
    std::cerr << "Error reading " << #obj << " from " << TEMPFOLDER + path     \
              << std::endl;                                                    \
    std::exit(1);                                                              \
  }
