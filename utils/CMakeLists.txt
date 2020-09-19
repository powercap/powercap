# SPDX-License-Identifier: BSD-3-Clause

# Binaries

add_executable(powercap-info powercap-info.c util-common.c)
target_link_libraries(powercap-info powercap)

add_executable(powercap-set powercap-set.c util-common.c)
target_link_libraries(powercap-set powercap)

install(TARGETS powercap-info powercap-set DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES man/man1/powercap-info.1 man/man1/powercap-set.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

# Deprecated Binaries

add_executable(rapl-info rapl-info.c util-common.c)
target_link_libraries(rapl-info powercap)

add_executable(rapl-set rapl-set.c util-common.c)
target_link_libraries(rapl-set powercap)

install(TARGETS rapl-info rapl-set DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES man/man1/rapl-info.1 man/man1/rapl-set.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
