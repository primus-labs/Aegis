add_compile_options(-fexceptions  -frtti)

function(create_executable target_name)
  set(sources ${ARGN})
  add_executable(${target_name} ${sources})
    
  target_link_libraries(${target_name}
    PUBLIC
    AegisRuntime
  )

  target_include_directories(${target_name} PUBLIC ${PROJECT_SOURCE_DIR}/include)
  target_include_directories(AegisRuntime PUBLIC ${PROJECT_SOURCE_DIR}/../backend)
  set(OPENFHE_LIBRARY_PATH ${PROJECT_SOURCE_DIR}/../third_party/openfhe/build/lib)
endfunction()

#target_link_options(test_runtime PRIVATE "-Wl,--verbose")
create_executable(test_runtime test_runtime.cpp)