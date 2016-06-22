# Powercap Sysfs C Bindings

This project provides the `powercap` library -- a generic C interface to the Linux powercap sysfs interface.
It includes an implementation for working with Intel Running Average Power Limit (RAPL).

## Prerequisites

Powercap (with the RAPL implementation) was released with Linux kernel 3.13.
You must be running this kernel or newer with the configs `CONFIG_POWERCAP` and `CONFIG_INTEL_RAPL` enabled.

If the `intel_rapl` kernel module is not loaded at startup, run with proper privileges:

```sh
modprobe intel_rapl
```

## Building

This project uses CMake.

To build, run:

``` sh
mkdir _build
cd _build
cmake ..
make
```

## Installing

To install, run with proper privileges:

``` sh
make install
```

On Linux, installation typically places libraries in `/usr/local/lib` and
header files in `/usr/local/include`.

## Uninstalling

Install must be run before uninstalling in order to have a manifest.
To uninstall, run with proper privileges:

``` sh
make uninstall
```

## Usage

The `powercap.h` interface provides read/write functions for generic powercap `zone` and `constraint` file sets.
Users are responsible for managing memory and populating the structs with file descriptors (e.g. code that wrap this interface performs zone/constraint discovery and file descriptor management).

The `powercap-rapl.h` interface discovers RAPL packages, power zones, and constraints (i.e. long\_term and short\_term constraints).
Users are responsible for managing memory, but the library will manage discovering, opening, and closing files within packages.

Basic lifecycle example:

```C
  // get number of processor sockets
  uint32_t npackages = powercap_rapl_get_num_packages();
  if (npackages == 0) {
    // no packages found (maybe the kernel module isn't loaded?)
    if (errno) {
      perror("powercap_rapl_get_num_packages")
    }
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
  // e.g. enable desired packages or power planes and get/set power caps...
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
