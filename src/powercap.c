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

int powercap_control_type_file_get_name(powercap_control_type_file type, char* buf, size_t size) {
  /* check type in case users pass bad int value instead of enum; int cast silences clang compiler */
  if (!buf || !size || (int) type < 0 || (int) type > POWERCAP_CONTROL_TYPE_FILE_ENABLED) {
    errno = EINVAL;
    return -errno;
  }
  return snprintf_control_type_file(buf, size, type);
}

int powercap_zone_file_get_name(powercap_zone_file type, char* buf, size_t size) {
  /* check type in case users pass bad int value instead of enum; int cast silences clang compiler */
  if (!buf || !size || (int) type < 0 || (int) type > POWERCAP_ZONE_FILE_NAME) {
    errno = EINVAL;
    return -errno;
  }
  return snprintf_zone_file(buf, size, type);
}

int powercap_constraint_file_get_name(powercap_constraint_file type, uint32_t constraint, char* buf, size_t size) {
  /* check type in case users pass bad int value instead of enum; int cast silences clang compiler */
  if (!buf || !size || (int) type < 0 || (int) type > POWERCAP_CONSTRAINT_FILE_NAME) {
    errno = EINVAL;
    return -errno;
  }
  return snprintf_constraint_file(buf, size, type, constraint);
}

int powercap_get_path(const char* control_type_name, const uint32_t* zones, uint32_t depth, char* buf, size_t size) {
  if (!control_type_name || !buf || !size || (depth && !zones)) {
    errno = EINVAL;
    return -errno;
  }
  return snprintf_base_path(buf, size, control_type_name, zones, depth);
}

int powercap_control_type_file_get_path(powercap_control_type_file type, const char* control_type_name, char* buf,
                                        size_t size) {
  /* check type in case users pass bad int value instead of enum; int cast silences clang compiler */
  if (!control_type_name || !buf || !size || (int) type < 0 || (int) type > POWERCAP_CONTROL_TYPE_FILE_ENABLED) {
    errno = EINVAL;
    return -errno;
  }
  return snprintf_control_type_file_path(buf, size, control_type_name, type);
}

int powercap_zone_file_get_path(powercap_zone_file type, const char* control_type_name, const uint32_t* zones,
                                uint32_t depth, char* buf, size_t size) {
  /* check type in case users pass bad int value instead of enum; int cast silences clang compiler */
  if (!control_type_name || !buf || !size || (int) type < 0 || (int) type > POWERCAP_ZONE_FILE_NAME ||
      (depth && !zones)) {
    errno = EINVAL;
    return -errno;
  }
  return snprintf_zone_file_path(buf, size, control_type_name, zones, depth, type);
}

int powercap_constraint_file_get_path(powercap_constraint_file type, const char* control_type_name,
                                      const uint32_t* zones, uint32_t depth, uint32_t constraint, char* buf,
                                      size_t size) {
  /* check type in case users pass bad int value instead of enum; int cast silences clang compiler */
  if (!control_type_name || !buf || !size || (int) type < 0 || (int) type > POWERCAP_CONSTRAINT_FILE_NAME ||
      (depth && !zones)) {
    errno = EINVAL;
    return -errno;
  }
  return snprintf_constraint_file_path(buf, size, control_type_name, zones, depth, constraint, type);
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
