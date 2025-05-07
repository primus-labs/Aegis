"""
Client Side
"""

from primus_aegis.fhe import Keyset as FHEKeyset

# generate prog_spec.json(compileResult.outputDirPath/compileResult.progSpecFileName) by test_1_compile.py
prog_spec_file = "./prog_spec.json"


# generate keys
fheKeyset = FHEKeyset.getInstance()
fheKeyset.generate(prog_spec_file=prog_spec_file)

# serialize keys(and context)
all_keys = fheKeyset.to_bytes()
print("len of all_keys:", len(all_keys), "type of all_keys:", type(all_keys))
pub_keys = fheKeyset.to_bytes(contain_sk=False)
print("len of pub_keys:", len(pub_keys), "type of pub_keys:", type(pub_keys))

with open("all_keys.bin", "wb") as f:
    f.write(all_keys)
with open("pub_keys.bin", "wb") as f:
    f.write(pub_keys)
