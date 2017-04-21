/**
 * I/O operations for powercap sysfs files.
 *
 * @author Connor Imes
 * @date 2016-06-01
 */
// for pread, pwrite
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "powercap.h"

/**
 * Expects and trusts the content of the file to be a string representation of a 64-bit unsigned value.
 * Read and load this value into the provided pointer.
 */
static int read_u64(int fd, uint64_t* val) {
  char data[24];
  if (pread(fd, data, sizeof(data), 0) > 0) {
    *val = strtoull(data, NULL, 0);
    return 0;
  }
  return -1;
}

/**
 * Expects the content of the file to be a string representation of an unsigned value.
 * Write this value into the file.
 */
static int write_u64(int fd, uint64_t val) {
  char data[24];
  snprintf(data, sizeof(data), "%"PRIu64, val);
  return pwrite(fd, data, sizeof(data), 0) <= 0 ? -1 : 0;
}

/**
 * Expects and trusts the content of the file to be a string representation of an int.
 * Read and load this value into the provided pointer.
 */
static int read_int(int fd, int* val) {
  char data[12];
  if (pread(fd, data, sizeof(data), 0) > 0) {
    *val = atoi(data);
    return 0;
  }
  return -1;
}

/**
 * Expects the content of the file to be a string representation of an unsigned value.
 * Write this value into the file.
 */
static int write_int(int fd, int val) {
  char data[16];
  snprintf(data, sizeof(data), "%d", val);
  return pwrite(fd, data, sizeof(data), 0) <= 0 ? -1 : 0;
}

/**
 * Read the contents of the file as a string up to 'size' chars.
 */
static ssize_t read_string(int fd, char* buf, size_t size) {
  ssize_t ret = pread(fd, buf, size, 0);
  if (ret > 0) {
    // force a terminating character in the buffer
    if ((size_t) ret < size) {
      buf[ret] = '\0';
      if (buf[ret - 1] == '\n') {
        // remove newline character from name
        buf[ret - 1] = '\0';
      }
    } else {
      buf[size - 1] = '\0';
    }
  }
  return ret;
}

int powercap_zone_file_get_name(powercap_zone_file type, char* buf, size_t size) {
  switch (type) {
    case POWERCAP_ZONE_FILE_MAX_ENERGY_RANGE_UJ:
      return snprintf(buf, size, "max_energy_range_uj");
    case POWERCAP_ZONE_FILE_ENERGY_UJ:
      return snprintf(buf, size, "energy_uj");
    case POWERCAP_ZONE_FILE_MAX_POWER_RANGE_UW:
      return snprintf(buf, size, "max_power_range_uw");
    case POWERCAP_ZONE_FILE_POWER_UW:
      return snprintf(buf, size, "power_uw");
    case POWERCAP_ZONE_FILE_ENABLED:
      return snprintf(buf, size, "enabled");
    case POWERCAP_ZONE_FILE_NAME:
      return snprintf(buf, size, "name");
    default:
      errno = EINVAL;
      return -1;
  }
}

int powercap_constraint_file_get_name(powercap_constraint_file type, uint32_t constraint, char* buf, size_t size) {
  switch (type) {
    case POWERCAP_CONSTRAINT_FILE_POWER_LIMIT_UW:
      return snprintf(buf, size, "constraint_%"PRIu32"_power_limit_uw", constraint);
    case POWERCAP_CONSTRAINT_FILE_TIME_WINDOW_US:
      return snprintf(buf, size, "constraint_%"PRIu32"_time_window_us", constraint);
    case POWERCAP_CONSTRAINT_FILE_MAX_POWER_UW:
      return snprintf(buf, size, "constraint_%"PRIu32"_max_power_uw", constraint);
    case POWERCAP_CONSTRAINT_FILE_MIN_POWER_UW:
      return snprintf(buf, size, "constraint_%"PRIu32"_min_power_uw", constraint);
    case POWERCAP_CONSTRAINT_FILE_MAX_TIME_WINDOW_US:
      return snprintf(buf, size, "constraint_%"PRIu32"_max_time_window_us", constraint);
    case POWERCAP_CONSTRAINT_FILE_MIN_TIME_WINDOW_US:
      return snprintf(buf, size, "constraint_%"PRIu32"_min_time_window_us", constraint);
    case POWERCAP_CONSTRAINT_FILE_NAME:
      return snprintf(buf, size, "constraint_%"PRIu32"_name", constraint);
    default:
      errno = EINVAL;
      return -1;
  }
}

int powercap_zone_get_max_energy_range_uj(const powercap_zone* zone, uint64_t* val) {
  return read_u64(zone->max_energy_range_uj, val);
}

int powercap_zone_get_energy_uj(const powercap_zone* zone, uint64_t* val) {
  return read_u64(zone->energy_uj, val);
}

int powercap_zone_reset_energy_uj(const powercap_zone* zone) {
  return write_u64(zone->energy_uj, 0);
}

int powercap_zone_get_max_power_range_uw(const powercap_zone* zone, uint64_t* val) {
  return read_u64(zone->max_power_range_uw, val);
}

int powercap_zone_get_power_uw(const powercap_zone* zone, uint64_t* val) {
  return read_u64(zone->power_uw, val);
}

int powercap_zone_set_enabled(const powercap_zone* zone, int val) {
  return write_int(zone->enabled, val);
}

int powercap_zone_get_enabled(const powercap_zone* zone, int* val) {
  return read_int(zone->enabled, val);
}

ssize_t powercap_zone_get_name(const powercap_zone* zone, char* buf, size_t size) {
  return read_string(zone->name, buf, size);
}

int powercap_constraint_set_power_limit_uw(const powercap_constraint* constraint, uint64_t val) {
  return write_u64(constraint->power_limit_uw, val);
}

int powercap_constraint_get_power_limit_uw(const powercap_constraint* constraint, uint64_t* val) {
  return read_u64(constraint->power_limit_uw, val);
}

int powercap_constraint_set_time_window_us(const powercap_constraint* constraint, uint64_t val) {
  return write_u64(constraint->time_window_us, val);
}

int powercap_constraint_get_time_window_us(const powercap_constraint* constraint, uint64_t* val) {
  return read_u64(constraint->time_window_us, val);
}

int powercap_constraint_get_max_power_uw(const powercap_constraint* constraint, uint64_t* val) {
  return read_u64(constraint->max_power_uw, val);
}

int powercap_constraint_get_min_power_uw(const powercap_constraint* constraint, uint64_t* val) {
  return read_u64(constraint->min_power_uw, val);
}

int powercap_constraint_get_max_time_window_us(const powercap_constraint* constraint, uint64_t* val) {
  return read_u64(constraint->max_time_window_us, val);
}

int powercap_constraint_get_min_time_window_us(const powercap_constraint* constraint, uint64_t* val) {
  return read_u64(constraint->min_time_window_us, val);
}

ssize_t powercap_constraint_get_name(const powercap_constraint* constraint, char* buf, size_t size) {
  return read_string(constraint->name, buf, size);
}
