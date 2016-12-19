cmake_minimum_required(VERSION 2.8.9)

project(powercap)
set(PROJECT_VERSION 0.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

include_directories(${PROJECT_SOURCE_DIR}/inc)

# Libraries

add_library(powercap src/powercap.c src/powercap-rapl.c)

# Tests

add_executable(powercap-test test/powercap-test.c)
target_link_libraries(powercap-test powercap)

add_executable(powercap-rapl-test test/powercap-rapl-test.c)
target_link_libraries(powercap-rapl-test powercap)

enable_testing()
macro(add_unit_test target)
  add_test(${target} ${EXECUTABLE_OUTPUT_PATH}/${target})
endmacro(add_unit_test)

add_unit_test(powercap-test)
# Requires a real system with root privileges
# add_unit_test(powercap-rapl-test)

# pkg-config

set(PKG_CONFIG_EXEC_PREFIX "\${prefix}")
set(PKG_CONFIG_LIBDIR "\${prefix}/lib")
set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include")
set(PKG_CONFIG_CFLAGS "-I\${includedir}")

set(PKG_CONFIG_NAME "${PROJECT_NAME}")
set(PKG_CONFIG_DESCRIPTION "C bindings to the Linux Power Capping Framework in sysfs")
set(PKG_CONFIG_LIBS "-L\${libdir} -lpowercap")
set(PKG_CONFIG_LIBS_PRIVATE "")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in
  ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/powercap.pc
)

# Install

install(TARGETS powercap DESTINATION lib)
install(FILES inc/powercap.h inc/powercap-rapl.h DESTINATION include)
install(DIRECTORY ${CMAKE_BINARY_DIR}/pkgconfig/ DESTINATION lib/pkgconfig)

# Uninstall

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
  @ONLY
)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
