/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * I/O operations for powercap sysfs files.
 *
 * @author Connor Imes
 * @date 2016-06-01
 */
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include "powercap.h"
#include "powercap-common.h"

int powercap_zone_file_get_name(powercap_zone_file type, char* buf, size_t size) {
  return zone_file_get_name(type, buf, size);
}

int powercap_constraint_file_get_name(powercap_constraint_file type, uint32_t constraint, char* buf, size_t size) {
  return constraint_file_get_name(type, constraint, buf, size);
}

#define VERIFY_ARG(arg) \
  if (!(arg)) { \
    errno = EINVAL; \
    return -errno; \
  }

int powercap_control_type_set_enabled(const powercap_control_type* control_type, int val) {
  VERIFY_ARG(control_type);
  return write_u64(control_type->enabled, (uint64_t) val);
}

int powercap_control_type_get_enabled(const powercap_control_type* control_type, int* val) {
  uint64_t enabled;
  int ret;
  VERIFY_ARG(control_type);
  VERIFY_ARG(val);
  if (!(ret = read_u64(control_type->enabled, &enabled))) {
    *val = enabled ? 1 : 0;
  }
  return ret;
}

int powercap_zone_get_max_energy_range_uj(const powercap_zone* zone, uint64_t* val) {
  VERIFY_ARG(zone);
  return read_u64(zone->max_energy_range_uj, val);
}

int powercap_zone_get_energy_uj(const powercap_zone* zone, uint64_t* val) {
  VERIFY_ARG(zone);
  return read_u64(zone->energy_uj, val);
}

int powercap_zone_reset_energy_uj(const powercap_zone* zone) {
  VERIFY_ARG(zone);
  return write_u64(zone->energy_uj, 0);
}

int powercap_zone_get_max_power_range_uw(const powercap_zone* zone, uint64_t* val) {
  VERIFY_ARG(zone);
  return read_u64(zone->max_power_range_uw, val);
}

int powercap_zone_get_power_uw(const powercap_zone* zone, uint64_t* val) {
  VERIFY_ARG(zone);
  return read_u64(zone->power_uw, val);
}

int powercap_zone_set_enabled(const powercap_zone* zone, int val) {
  VERIFY_ARG(zone);
  return write_u64(zone->enabled, (uint64_t) val);
}

int powercap_zone_get_enabled(const powercap_zone* zone, int* val) {
  uint64_t enabled;
  int ret;
  VERIFY_ARG(zone);
  VERIFY_ARG(val);
  if (!(ret = read_u64(zone->enabled, &enabled))) {
    *val = enabled ? 1 : 0;
  }
  return ret;
}

ssize_t powercap_zone_get_name(const powercap_zone* zone, char* buf, size_t size) {
  VERIFY_ARG(zone);
  return read_string(zone->name, buf, size);
}

int powercap_constraint_set_power_limit_uw(const powercap_constraint* constraint, uint64_t val) {
  VERIFY_ARG(constraint)
  return write_u64(constraint->power_limit_uw, val);
}

int powercap_constraint_get_power_limit_uw(const powercap_constraint* constraint, uint64_t* val) {
  VERIFY_ARG(constraint)
  return read_u64(constraint->power_limit_uw, val);
}

int powercap_constraint_set_time_window_us(const powercap_constraint* constraint, uint64_t val) {
  VERIFY_ARG(constraint)
  return write_u64(constraint->time_window_us, val);
}

int powercap_constraint_get_time_window_us(const powercap_constraint* constraint, uint64_t* val) {
  VERIFY_ARG(constraint)
  return read_u64(constraint->time_window_us, val);
}

int powercap_constraint_get_max_power_uw(const powercap_constraint* constraint, uint64_t* val) {
  VERIFY_ARG(constraint)
  return read_u64(constraint->max_power_uw, val);
}

int powercap_constraint_get_min_power_uw(const powercap_constraint* constraint, uint64_t* val) {
  VERIFY_ARG(constraint)
  return read_u64(constraint->min_power_uw, val);
}

int powercap_constraint_get_max_time_window_us(const powercap_constraint* constraint, uint64_t* val) {
  VERIFY_ARG(constraint)
  return read_u64(constraint->max_time_window_us, val);
}

int powercap_constraint_get_min_time_window_us(const powercap_constraint* constraint, uint64_t* val) {
  VERIFY_ARG(constraint)
  return read_u64(constraint->min_time_window_us, val);
}

ssize_t powercap_constraint_get_name(const powercap_constraint* constraint, char* buf, size_t size) {
  VERIFY_ARG(constraint)
  return read_string(constraint->name, buf, size);
}
