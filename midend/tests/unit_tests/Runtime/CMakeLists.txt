add_compile_options(-fexceptions  -frtti)

function(create_executable target_name)
  set(sources ${ARGN})
  add_executable(${target_name} ${sources})

  set(_origin_prefix "\$ORIGIN")
  if(APPLE)
    set(_origin_prefix "@loader_path")
  endif()
  target_link_options(${target_name} PRIVATE "-Wl,-rpath,${_origin_prefix}" "-Wl,-rpath,/usr/local/lib")
    
  target_link_libraries(${target_name}
    PUBLIC
    AegisRuntime
  )

  target_include_directories(${target_name} PUBLIC ${PROJECT_SOURCE_DIR}/include)
  target_include_directories(AegisRuntime PUBLIC ${PROJECT_SOURCE_DIR}/../backend)
  set(OPENFHE_LIBRARY_PATH ${PROJECT_SOURCE_DIR}/../third_party/openfhe/build/lib /usr/local/lib)
endfunction()

create_executable(runtime_test test_runtime.cpp)