# SPDX-License-Identifier: BSD-3-Clause

macro(add_unit_test target)
  add_test(${target} ${target})
endmacro(add_unit_test)

add_executable(powercap-common-test powercap-common-test.c ${PROJECT_SOURCE_DIR}/src/powercap-common.c)
target_include_directories(powercap-common-test PRIVATE ${PROJECT_SOURCE_DIR}/inc)
add_unit_test(powercap-common-test)

add_executable(powercap-test powercap-test.c)
target_link_libraries(powercap-test PRIVATE powercap)
add_unit_test(powercap-test)

add_executable(powercap-rapl-test powercap-rapl-test.c)
target_link_libraries(powercap-rapl-test PRIVATE powercap)
# Requires a real system with root privileges
# add_unit_test(powercap-rapl-test)

add_executable(powercap-sysfs-test powercap-sysfs-test.c)
target_link_libraries(powercap-sysfs-test PRIVATE powercap)
add_unit_test(powercap-sysfs-test)
