# Release Notes

## [Unreleased]

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

[Unreleased]: https://github.com/powercap/powercap/compare/v0.1.1...HEAD
[v0.1.1]: https://github.com/powercap/powercap/compare/v0.1.0...v0.1.1
[#1]: https://github.com/powercap/powercap/issues/1
