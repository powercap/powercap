# Powercap Sysfs C Bindings and Utilities

This project provides the `powercap` library -- a generic C interface to the Linux power capping framework (sysfs interface).
It includes an implementation for working with Intel Running Average Power Limit (RAPL).

It also provides the following applications:

* `powercap-info` - view powercap control type hierarchies or zone/constraint-specific configurations
* `powercap-set` - set powercap control type zone/constraint-specific configurations

These bindings were originally created for use with [RAPLCap](https://github.com/powercap/raplcap), but can be used independently.
See the RAPLCap project for a more general interface for managing RAPL power caps, including other command line utilities.

If using this project for other scientific works or publications, please reference:

* Connor Imes, Huazhe Zhang, Kevin Zhao, Henry Hoffmann. "CoPPer: Soft Real-time Application Performance Using Hardware Power Capping". In: IEEE International Conference on Autonomic Computing (ICAC). 2019. DOI: https://doi.org/10.1109/ICAC.2019.00015
  <details>
  <summary>BibTex</summary>

  ```BibTex
  @inproceedings{imes2019copper,
    author={Imes, Connor and Zhang, Huazhe and Zhao, Kevin and Hoffmann, Henry},
    booktitle={2019 IEEE International Conference on Autonomic Computing (ICAC)},
    title={{CoPPer}: Soft Real-Time Application Performance Using Hardware Power Capping},
    year={2019},
    pages={31-41},
    doi={10.1109/ICAC.2019.00015}
  }
  ```

  </details>


## Prerequisites

Powercap (with the RAPL implementation) was released with Linux kernel 3.13.
You must be running this kernel or newer with the configs `CONFIG_POWERCAP` and `CONFIG_INTEL_RAPL` enabled.

To use the `intel-rapl` control type, ensure that the appropriate kernel module is loaded.
Run with proper privileges:

```sh
modprobe intel_rapl_msr
```

Or on kernels older than 5.3:

```sh
modprobe intel_rapl
```


## Power Capping

Modern hardware is constrained by power and temperature limitations, often quantified as Thermal Design Power.
In processors and other clock-driven hardware components, power consumption `P` is proportional to capacitance `C`, the square of the voltage `V`, and clock frequency `f`: `P ~ C * V^2 * f`.
A popular mechanism for balancing performance and power consumption is Dynamic Voltage and Frequency Scaling (DVFS).
For compute-bound applications, DVFS provides a linear relationship between frequency and performance.
However, power is non-linear with frequency since an increase in frequency also requires an increase in voltage.

Although the relationship between performance and power is more difficult to model, hardware can be better at optimizing voltage and frequency than software while still respecting a power cap over a time window.
Power capping allows a system administrator to configure an upper limit on the power consumption of various hardware components while letting the hardware more efficiently manage voltage and frequency.
Setting a power cap does *NOT* imply that the component will actually consume that power, only that it will not violate that limit on average over the specified time window.


## Usage

### Applications

See the man pages or run the applications with the `-h` or `--help` option for instructions.

### Library

First, there is the `powercap-sysfs.h` interface for reading/writing to sysfs without the need to maintain state.
This is reasonable for simple use cases.
See the header files for documentation.

The `powercap.h` interface provides read/write functions for generic powercap `zone` and `constraint` file sets.
Users are responsible for managing memory and populating the structs with file descriptors (e.g., code that wrap this interface performs zone/constraint discovery and file descriptor management).

The `powercap-rapl.h` interface discovers RAPL instances, power zones, and constraints (i.e., long\_term and short\_term constraints).
Users are responsible for managing memory, but the library will manage discovering, opening, and closing files within instances.

Basic lifecycle example:

```C
  // get number of top-level (parent) RAPL instances
  uint32_t count = powercap_rapl_get_num_instances();
  if (count == 0) {
    // none found (maybe the kernel module isn't loaded?)
    perror("powercap_rapl_get_num_instances")
    return -1;
  }
  powercap_rapl_pkg* pkgs = malloc(count * sizeof(powercap_rapl_pkg));
  // initialize
  uint32_t i;
  for (i = 0; i < count; i++) {
    if (powercap_rapl_init(i, &pkgs[i], 0)) {
      // could be that you don't have write privileges
      perror("powercap_rapl_init");
      return -1;
    }
  }
  // do a bunch of stuff with the interface here,
  // e.g., enable desired zones and get/set power caps...
  // now cleanup
  for (i = 0; i < count; i++) {
    if (powercap_rapl_destroy(&pkgs[i])) {
      perror("powercap_rapl_destroy");
    }
  }
  free(pkgs);
```

### Additional Comments

The interfaces do _NOT_ guarantee that values are actually accepted by the kernel, they only notice errors if I/O operations fail.
It is recommended that, at least during development/debugging, users read back to see if their write operations were successful.

Additionally, the kernel sysfs bindings (and thus the `powercap-rapl` interface) do _NOT_ guarantee that RAPL instances are presented in any particular order.
For example, the first instance (sysfs directory `intel-rapl:0`) on a dual-socket system may actually provide access to `package-1` instead of `package-0`, and vice versa.
In cases where order matters, e.g., when sockets are managed asymmetrically, the user is responsible for ensuring that the correct powercap instance is being operated on, e.g., by checking its name with `powercap_rapl_get_name(...)`.
More concretely, in the example above, `powercap_rapl_get_name(&pkgs[0], POWERCAP_RAPL_ZONE_PACKAGE, ...)` gives name `package-1`, and `powercap_rapl_get_name(&pkgs[1], POWERCAP_RAPL_ZONE_PACKAGE, ...)` is `package-0`.
It might be helpful to sort the `pkgs` array *after* initialization (see the `powercap` implementation of [RAPLCap](https://github.com/powercap/raplcap) for an example).

Finally, the `powercap-rapl` interface exposes functions for files that are not (currently) supported by RAPL in order to be compliant with the powercap interface.
Use the `powercap_rapl_is_zone_file_supported(...)` and `powercap_rapl_is_constraint_file_supported(...)` functions to check in advance if you are unsure if a zone or constraint file is supported.
Furthermore, files may exist but always return an error code for some zones or constraints, e.g., the constraint `max_power_uw` file (`powercap_rapl_get_max_power_uw(...)`) for zones other than `POWERCAP_RAPL_ZONE_PACKAGE`.


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

To create a shared object library as a release build, specify for cmake:

``` sh
cmake .. -DBUILD_SHARED_LIBS=On -DCMAKE_BUILD_TYPE=Release
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
