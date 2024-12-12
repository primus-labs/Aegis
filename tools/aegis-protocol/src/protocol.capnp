# -----------------------------------------------------------------------------------------------------
# Aegis Protocol
# 
# The following document describes the communication protocol used to store and exchange data 
# with applications within the Aegis framework.
#
# -----------------------------------------------------------------------------------------------------

@0xfdae028881631719;

struct Shape{ 
  dimensions @0 :List(UInt32);  # The dimensions of the value.
}


struct Payload{
  data @0 :List(Data);  # The binary data of the payload
}


struct RawInfo{
  shape @0 :Shape;      # The shape of the tensor.
  isSigned @1 :Bool;    # The signedness of the value.
}


struct RawData {
  payload @0 :Payload;  # The binary payload
  rawInfo @1 :RawInfo;  # The informations to parse the binary payload.
}