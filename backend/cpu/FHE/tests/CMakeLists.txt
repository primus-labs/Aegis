function(create_executable target_name)
  set(sources ${ARGN})
  add_executable(${target_name} ${sources})
    
  target_link_libraries(${target_name}
    PRIVATE
    aegis_cpu
    AegisCommon
    OPENFHEcore
    OPENFHEpke
  )

  target_include_directories(${target_name} PUBLIC ${AEGIS_CPU_FHE_INCLUDE_DIR})
  target_include_directories(${target_name} PUBLIC ${PROJECT_SOURCE_DIR}/include)
  target_include_directories(${target_name} PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/src/pke/include)
  target_include_directories(${target_name} PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/src/binfhe/include)
  target_include_directories(${target_name} PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/src/core/include)
  target_include_directories(${target_name} PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/build/src/core)
  target_include_directories(${target_name} PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/third-party/cereal/include)

  target_link_directories(${target_name} PRIVATE ${PROJECT_SOURCE_DIR}/../third_party/openfhe/build/lib)
endfunction()

create_executable(fhe_test main.cpp)
create_executable(sim_client1 sim_client1.cpp)
create_executable(sim_server sim_server.cpp)
create_executable(sim_client2 sim_client2.cpp)
