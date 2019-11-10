# Release Notes

## [Unreleased]
### Added
 * This RELEASES.md file
 * Multiarch support (use GNU standard installation directories)
 * Additional documentation in README

### Changed
 * Increased minimum CMake version from 2.8 to 2.8.5 to support GNUInstallDirs
 * More pedantic man page source formatting
 * RAPL sysfs 'pkg' parameter names changed to 'zone' to improve generality
 * RAPL 'package' parameter names changed to 'id' to improve generality

### Deprecated
 * Function 'rapl_sysfs_pkg_exists' in powercap-rapl-sysfs.h - use 'rapl_sysfs_zone_exists' instead
 * Function 'rapl_sysfs_sz_exists' in powercap-rapl-sysfs.h - use 'rapl_sysfs_zone_exists' instead

### Removed
 * Removed private symbol exports in shared object library (already patched in Debian)

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
