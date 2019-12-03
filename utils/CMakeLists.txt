# SPDX-License-Identifier: BSD-3-Clause

# Binaries

add_executable(powercap-info powercap-info.c util-common.c)
target_link_libraries(powercap-info powercap)

add_executable(powercap-set powercap-set.c util-common.c)
target_link_libraries(powercap-set powercap)

add_executable(rapl-info rapl-info.c util-common.c)
target_link_libraries(rapl-info powercap)

add_executable(rapl-set rapl-set.c util-common.c)
target_link_libraries(rapl-set powercap)

# Install

install(TARGETS powercap-info powercap-set rapl-info rapl-set DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY man/ DESTINATION ${CMAKE_INSTALL_MANDIR})
