from primus_aegis.fhe import Keyset as FHEKeyset

print("Simulate Server")

# load keys and initialize context
fheKeyset = FHEKeyset.getInstance()
with open("pub_keys.bin", "rb") as f:
    pub_keys = f.read()
    fheKeyset.from_bytes(pub_keys)

    # for test
    if True:
        all_keys = fheKeyset.to_bytes()
        print("(test1) len of all_keys:", len(all_keys))
        pub_keys = fheKeyset.to_bytes(contain_sk=False)
        print("(test2) len of pub_keys:", len(pub_keys))
