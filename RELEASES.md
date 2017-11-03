# Release Notes

## [Unreleased]
### Added
 * This RELEASES.md file
 * Multiarch support (use GNU standard installation directories)

### Changed
 * Increased minimum CMake version from 2.8 to 2.8.5 to support GNUInstallDirs

### Removed
 * Removed private symbol exports in shared object library
  * Note: This is already patched in Debian; no symbols will be lost there


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
