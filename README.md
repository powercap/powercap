# Powercap Sysfs C Bindings and Utilities

This project provides the `powercap` library -- a generic C interface to the Linux power capping framework (sysfs interface).
It includes an implementation for working with Intel Running Average Power Limit (RAPL).

It also provides the following applications:

* `powercap-info` - view powercap control type hierarchies or zone/constraint-specific configurations
* `powercap-set` - set powercap control zone/constraint-specific configurations
* `rapl-info` - view Intel RAPL hierarchies or zone/constraint-specific configurations
* `rapl-set` - set Intel RAPL zone/constraint-specific configurations

These bindings were originally created for use with [RAPLCap](https://github.com/powercap/raplcap), but can be used independently.
See the RAPLCap project for a more general interface for managing RAPL power caps, including other command line utilities.

## Prerequisites

Powercap (with the RAPL implementation) was released with Linux kernel 3.13.
You must be running this kernel or newer with the configs `CONFIG_POWERCAP` and `CONFIG_INTEL_RAPL` enabled.

If the `intel_rapl` kernel module is not loaded at startup, run with proper privileges:

```sh
modprobe intel_rapl
```


## Usage

### Applications

See the man pages or run the applications with the `-h` or `--help` option for instructions.

### Library

First, there are the `powercap-sysfs.h` and `powercap-rapl-sysfs.h` interfaces for reading/writing to sysfs without the need to maintain state.
These are reasonable for simple use cases.
See the header files for documentation.

The `powercap.h` interface provides read/write functions for generic powercap `zone` and `constraint` file sets.
Users are responsible for managing memory and populating the structs with file descriptors (e.g., code that wrap this interface performs zone/constraint discovery and file descriptor management).

The `powercap-rapl.h` interface discovers RAPL packages, power zones, and constraints (i.e., long\_term and short\_term constraints).
Users are responsible for managing memory, but the library will manage discovering, opening, and closing files within packages.

Basic lifecycle example:

```C
  // get number of processor sockets
  uint32_t npackages = powercap_rapl_get_num_packages();
  if (npackages == 0) {
    // no packages found (maybe the kernel module isn't loaded?)
    perror("powercap_rapl_get_num_packages")
    return -1;
  }
  powercap_rapl_pkg* pkgs = malloc(npackages * sizeof(powercap_rapl_pkg));
  // initialize
  uint32_t i;
  for (i = 0; i < npackages; i++) {
    if (powercap_rapl_init(i, &pkgs[i], 0)) {
      // could be that you don't have write privileges
      perror("powercap_rapl_init");
      return -1;
    }
  }
  // do a bunch of stuff with the interface here,
  // e.g., enable desired packages or power planes and get/set power caps...
  // now cleanup
  for (i = 0; i < npackages; i++) {
    if (powercap_rapl_destroy(&pkgs[i])) {
      perror("powercap_rapl_destroy");
    }
  }
  free(pkgs);
```

Note that the interfaces do not guarantee that values are actually accepted by the kernel, they only notice errors if I/O operations fail.
It is recommended that, at least during development/debugging, users read back to see if their write operations were successful.


## Building

### Compiling

This project uses CMake.

To build, run:

``` sh
mkdir _build
cd _build
cmake ..
make
```


### Installing

To install, run with proper privileges:

``` sh
make install
```

On Linux, installation typically places libraries in `/usr/local/lib` and
header files in `/usr/local/include`.

### Uninstalling

Install must be run before uninstalling in order to have a manifest.
To uninstall, run with proper privileges:

``` sh
make uninstall
```


## Project Source

Find this and related project sources at the [powercap organization on GitHub](https://github.com/powercap).  
This project originates at: https://github.com/powercap/powercap

Bug reports and pull requests for bug fixes and enhancements are welcome.


## License

This project is developed by Connor Imes.
It is released under the 3-Clause BSD License.

## Thanks

Special thanks to Henry Hoffmann (University of Chicago) and Steven Hofmeyr (Lawrence Berkeley National Laboratory) for advising and supporting projects that this code was originally developed for.
