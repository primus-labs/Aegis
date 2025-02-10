# add_aegis_dialect(dialect dialect_namespace)
add_aegis_dialect(FHE fhe)

# add_aegis_doc(doc_filename(.td) output_file(.md) output_directory command)
add_aegis_doc(FHEDialect FHEDialect FHE/ -gen-dialect-doc)
add_aegis_doc(FHEOps FHEOps FHE/ -gen-op-doc -dialect=FHE)
add_aegis_doc(FHETypes FHETypes FHE/ -gen-typedef-doc -dialect=FHE)
