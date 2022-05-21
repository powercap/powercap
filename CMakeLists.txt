# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.12)

project(powercap VERSION 0.6.1
                 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
add_compile_options(-Wall)

include(GNUInstallDirs)

# See powercap-common.h for enumeration
set(POWERCAP_LOG_LEVEL 4 CACHE STRING "Set the log level: 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR, 4=OFF (default)")

set(POWERCAP_CMAKE_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/powercap)

# Libraries

add_library(powercap src/powercap.c
                     src/powercap-sysfs.c
                     src/powercap-rapl.c
                     src/powercap-rapl-sysfs.c
                     src/powercap-common.c)
target_include_directories(powercap PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
                                           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}>)
set_target_properties(powercap PROPERTIES PUBLIC_HEADER "inc/powercap.h;inc/powercap-sysfs.h;inc/powercap-rapl.h;inc/powercap-rapl-sysfs.h")
target_compile_definitions(powercap PRIVATE POWERCAP_LOG_LEVEL=${POWERCAP_LOG_LEVEL})
if (BUILD_SHARED_LIBS)
  set_target_properties(powercap PROPERTIES VERSION ${PROJECT_VERSION}
                                            SOVERSION ${PROJECT_VERSION_MAJOR})
endif()
install(TARGETS powercap
        EXPORT PowercapTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                COMPONENT Powercap_Runtime
                NAMELINK_COMPONENT Powercap_Development
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                COMPONENT Powercap_Development
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
                      COMPONENT Powercap_Development)

# pkg-config

set(PKG_CONFIG_EXEC_PREFIX "\${prefix}")
if (IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
  set(PKG_CONFIG_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
else()
  set(PKG_CONFIG_LIBDIR "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
endif()
if (IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
  set(PKG_CONFIG_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}")
else()
  set(PKG_CONFIG_INCLUDEDIR "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}")
endif()
set(PKG_CONFIG_CFLAGS "-I\${includedir}")

set(PKG_CONFIG_NAME "${PROJECT_NAME}")
set(PKG_CONFIG_DESCRIPTION "C bindings to the Linux Power Capping Framework in sysfs")
set(PKG_CONFIG_LIBS "-L\${libdir} -lpowercap")
set(PKG_CONFIG_LIBS_PRIVATE "")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in
  ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/powercap.pc
)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
        COMPONENT Powercap_Development)

# CMake package helper

include(CMakePackageConfigHelpers)

set(CONFIG_TARGETS_FILE PowercapTargets.cmake)
# While we (currently) always build and install utils, making their import optional makes downstream packaging easier.
# E.g., cmake package configs belong in the Debian dev package, which depends on the lib---but not the utils---package.
set(CONFIG_SUPPORTED_COMPONENTS Utils)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/PowercapConfig.cmake
  INSTALL_DESTINATION ${POWERCAP_CMAKE_CONFIG_INSTALL_DIR}
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/PowercapConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/PowercapConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/PowercapConfigVersion.cmake
        DESTINATION ${POWERCAP_CMAKE_CONFIG_INSTALL_DIR}
        COMPONENT Powercap_Development)
install(EXPORT PowercapTargets
        DESTINATION ${POWERCAP_CMAKE_CONFIG_INSTALL_DIR}
        NAMESPACE Powercap::
        COMPONENT Powercap_Development)

# Uninstall

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
  @ONLY
)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

# Tests and Utilities

enable_testing()
add_subdirectory(test)
add_subdirectory(utils)
