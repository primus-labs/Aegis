# add_aegis_dialect(dialect dialect_namespace)
add_aegis_dialect(Secret secret)

# add_aegis_doc(doc_filename(.td) output_file(.md) output_directory command)
add_aegis_doc(SecretDialect SecretDialect Secret/ -gen-dialect-doc)
add_aegis_doc(SecretOps SecretOps Secret/ -gen-op-doc -dialect=Secret)
add_aegis_doc(SecretTypes SecretTypes Secret/ -gen-typedef-doc -dialect=Secret)