# Release Notes

## [Unreleased]

### Added

* Binaries 'powercap-{info,set}-intel-rapl' (more faithful to Linux power capping framework interface than 'rapl-{info,set}')
* Top-level 'powercap_control_type' type to powercap.h with file getters/setters (for 'enabled' field)
* Functions 'powercap_sysfs_control_type_{get,set}_enabled' in powercap-sysfs.h
* Function 'powercap_rapl_get_num_instances' in powercap-rapl.h
* Argument '--enabled' to powercap-{info,set} for getting/setting control type enabled field
* Argument '--nconstraints' to powercap-info for getting the number of zone constraints

### Changed

* Increased minimum CMake version from 2.8.5 to 2.8.12 to support target_compile_definitions
* Updated 'powercap-{info,set}' man pages
* Argument '--zone' for powercap-set no longer required due to introduction of '--enabled' argument

### Deprecated

* Binaries rapl-{info,set} - use powercap-{info,set}-intel-rapl instead
* Interface powercap-rapl-sysfs.h - use powercap-sysfs.h directly instead
* Function 'powercap_rapl_get_num_packages' in powercap-rapl.h - use 'powercap_rapl_get_num_instances' instead

### Fixed

* Fixed [#4]: powercap-rapl now checks if parent zone is PACKAGE or PSYS


## [v0.2.0] - 2019-12-03

### Added

* This RELEASES.md file
* Multiarch support (use GNU standard installation directories)
* Additional documentation in README
* Function 'rapl_sysfs_zone_exists' in powercap-rapl-sysfs.h
* Command line long option '--zone' to rapl-info and rapl-set
* Functions to powercap-rapl-sysfs.h allowed by the powercap API that RAPL doesn't support (but could in the future)
* Options to rapl-info and rapl-set to support optional powercap features not currently implemented in RAPL

### Changed

* Increased minimum CMake version from 2.8 to 2.8.5 to support GNUInstallDirs
* More pedantic man page source formatting
* RAPL sysfs 'pkg' parameter names changed to 'zone' to improve generality
* RAPL 'package' parameter names changed to 'id' to improve generality
* On failure, binaries now exit with positive error codes instead of negative values

### Deprecated

* Function 'rapl_sysfs_pkg_exists' in powercap-rapl-sysfs.h - use 'rapl_sysfs_zone_exists' instead
* Function 'rapl_sysfs_sz_exists' in powercap-rapl-sysfs.h - use 'rapl_sysfs_zone_exists' instead
* Command line long option '--package' for rapl-info and rapl-set - use '--zone' instead

### Removed

* Removed private symbol exports in shared object library (already patched in Debian)
* Undocumented (and unintended) --package long option from powercap-set

### Fixed

* Fixed [#1]: Kernel uses hexadecimal numbers in directory paths, not decimal


## [v0.1.1] - 2017-09-21

### Added

* Added stateless interfaces to sysfs
* Added powercap-info, powercap-set, rapl-info, and rapl-set binaries and man pages
* Added VERSION and SOVERSION to shared object libraries

### Changed

* Update license to use author as copyright holder


## v0.1.0 - 2017-06-09

### Added

* Initial public release

[Unreleased]: https://github.com/powercap/powercap/compare/v0.2.0...HEAD
[v0.2.0]: https://github.com/powercap/powercap/compare/v0.1.1...v0.2.0
[v0.1.1]: https://github.com/powercap/powercap/compare/v0.1.0...v0.1.1
[#4]: https://github.com/powercap/powercap/issues/4
[#1]: https://github.com/powercap/powercap/issues/1
