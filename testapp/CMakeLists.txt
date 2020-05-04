add_executable(testapp testapp.cpp testpipeline.cpp)
hsh_sources(testpipeline.cpp)

target_link_libraries(testapp PUBLIC boo2)
target_hsh(testapp)

if(NX)
  add_nro_target(testapp)
  add_nso_target(testapp)
endif()
