# -----------------------------------------------------------------------------------------------------
# Aegis Protocol
# 
# The following document describes the communication protocol used to store and exchange data 
# with applications within the Aegis framework.
#
# -----------------------------------------------------------------------------------------------------

@0xfdae028881631719;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("aegisprotocol");

struct Shape{ 
  dimensions @0 :List(UInt32);  # The dimensions of the value.
}


struct Payload{
  data @0 :Data;  # The binary data of the payload.
}


struct RawInfo{
  shape @0 :Shape;      # The shape of the tensor.
  isSigned @1 :Bool;    # The signedness of the value.
  rawSize @2 :UInt32;   # The number of raw data elements. 
}


struct RawType{
  isCiphertext  @0 :Bool;
  isPlaintext @1 :Bool;
  isConstant @2 :Bool;
}


struct RawData {
  payload @0 :Payload;  # The binary payload
  rawInfo @1 :RawInfo;  # The informations to parse the binary payload.
  rawType @2 :RawType;  # The different possible type of raw data.
}


struct KeyInfo {
  polyModDegree @0 :UInt32;       # poly modulus degree.
  coffModCh @1 :List(UInt32);     # coff modulus chain.
  scale @2  :UInt32;              # scale factor.
  multDepth @3 :UInt32;           # Multiplication depth
  scaleModSize @4 :UInt32;        # Scale modulus size
  batchSize @5 :UInt32;           # Batch size
  galoisIndices @6 :List(Int32);  # Index list for Galois Key
  enableBootstrapping @7 :Bool;   # Whether to enable bootstrapping
  numSlot @8 :UInt32;             # number of slots
}


struct Functions {
  functions @0 :List(Function);   # The function overview.
}


struct Function {
  name @0 :Text;                # The name of the function.
  inputs @1 :List(FuncParam);   # function input.
  outputs @2 :List(FuncParam);  # function output.
}


struct FuncParam {
  name @0 :Text;    # The name of the input parameter or output.
  shape @1 :Shape;  # The shape of the params.
  type @2 :Bool;    # clear type or encrypt type.
}


struct StatsInfo {
  mulCount @0 :UInt32;  # multiply counts.
  rotCount @1 :UInt32;  # rotate counts.
  bsCount @2  :UInt32;  # boostraping counts.
  cmpCount @3 :UInt32;  # compare counts.
  selCount @4 :UInt32;  # select counts.
  level @5    :UInt32;  # encrypted levels.
}


struct ProgSpec {
  keyInfo @0 :KeyInfo;        # keyset informations
  funcsInfo @1 :Functions;    # function informations
  statsInfo @2 :StatsInfo;  # statistic informations
}
