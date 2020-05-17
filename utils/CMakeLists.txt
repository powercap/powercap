# SPDX-License-Identifier: BSD-3-Clause

function(POWERCAP_UTIL_ADD app control_type)
  add_executable(${app}-${control_type} ${app}.c util-common.c)
  target_compile_definitions(${app}-${control_type} PRIVATE POWERCAP_CONTROL_TYPE="${control_type}")
  target_link_libraries(${app}-${control_type} powercap)

  string(REPLACE "-" "\\-" MAN_CONTROL_TYPE "${control_type}")
  string(TOUPPER "${MAN_CONTROL_TYPE}" MAN_CONTROL_TYPE_UPPER)
  configure_file(
    ${CMAKE_SOURCE_DIR}/utils/man/man1/${app}.1.in
    ${CMAKE_CURRENT_BINARY_DIR}/man/man1/${app}-${control_type}.1
    @ONLY
  )

  install(TARGETS ${app}-${control_type}
          DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/man/man1/${app}-${control_type}.1
          DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endfunction(POWERCAP_UTIL_ADD)

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
