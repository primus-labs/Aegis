#pragma once
#include <cinttypes>
#include <vector>

struct MockKeyInfo {
  uint32_t polyModDegree;             // poly modulus degree.
  std::vector<uint32_t> coffModCh;    // coff modulus chain.
  uint32_t scale;                     // scale factor.
  uint32_t multDepth;                 // Multiplication depth
  uint32_t scaleModSize;              // Scale modulus size
  uint32_t batchSize;                 // Batch size (number of slots)
  std::vector<int32_t> galoisIndices; // Index list for Galois Key
  bool enableBootstrapping;           // Whether to enable bootstrapping
};

#include "Common/Protocol.h"
#include <protocol.capnp.h>

static void _buildKeyInfo(const MockKeyInfo &mockKeyInfo,
                          ProtoMessage<aegisprotocol::KeyInfo> &keyInfo) {
  keyInfo.asBuilder().setPolyModDegree(mockKeyInfo.polyModDegree);
  keyInfo.asBuilder().setScale(mockKeyInfo.scale);
  keyInfo.asBuilder().setMultDepth(mockKeyInfo.multDepth);
  keyInfo.asBuilder().setScaleModSize(mockKeyInfo.scaleModSize);
  keyInfo.asBuilder().setBatchSize(mockKeyInfo.batchSize);
  keyInfo.asBuilder().setEnableBootstrapping(mockKeyInfo.enableBootstrapping);

  auto coff = mockKeyInfo.coffModCh;
  auto coffModCh = keyInfo.asBuilder().initCoffModCh(coff.size());
  for (size_t i = 0; i < coff.size(); ++i) {
    coffModCh.set(i, coff[i]);
  }
  auto galois = mockKeyInfo.galoisIndices;
  auto galoisIndices = keyInfo.asBuilder().initGaloisIndices(galois.size());
  for (size_t i = 0; i < galois.size(); ++i) {
    galoisIndices.set(i, galois[i]);
  }
}

static void mockKeyInfo01(ProtoMessage<aegisprotocol::KeyInfo> &keyInfo) {
  MockKeyInfo mockKeyInfo;
  mockKeyInfo.multDepth = 1;
  mockKeyInfo.scaleModSize = 50;
  mockKeyInfo.batchSize = 8;
  mockKeyInfo.galoisIndices = {1, -2};

  _buildKeyInfo(mockKeyInfo, keyInfo);
}
