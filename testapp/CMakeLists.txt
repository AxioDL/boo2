if(NX)
  set(TESTAPP_NAME testapp)
else()
  set(TESTAPP_NAME testapp-pre)
endif()

add_executable(${TESTAPP_NAME} testapp.cpp testpipeline.cpp)
hsh_sources(testpipeline.cpp)

target_link_libraries(${TESTAPP_NAME} PUBLIC boo2)
target_hsh(${TESTAPP_NAME})

if(NX)
  add_nro_target(${TESTAPP_NAME})
  add_nso_target(${TESTAPP_NAME})
else()
  hsh_add_executable(testapp ${TESTAPP_NAME})
endif()
