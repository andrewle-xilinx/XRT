set(TESTNAME "00_hello")

add_executable(${TESTNAME} main.cpp)
target_link_libraries(${TESTNAME} PRIVATE ${xrt_core_LIBRARY} ${xrt_coreutil_LIBRARY})

if (NOT WIN32)
  target_link_libraries(${TESTNAME} PRIVATE ${uuid_LIBRARY} pthread)
endif(NOT WIN32)

install(TARGETS ${TESTNAME}
  RUNTIME DESTINATION ${INSTALL_DIR}/${TESTNAME})
