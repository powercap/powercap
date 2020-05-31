/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * RAPL implementation of powercap.
 *
 * @author Connor Imes
 * @date 2016-05-12
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "powercap.h"
#include "powercap-common.h"
#include "powercap-rapl.h"
#include "powercap-sysfs.h"

#ifndef MAX_NAME_SIZE
  #define MAX_NAME_SIZE 64
#endif

#define CONTROL_TYPE "intel-rapl"

#define CONSTRAINT_NUM_LONG 0
#define CONSTRAINT_NUM_SHORT 1

#define CONSTRAINT_NAME_LONG "long_term"
#define CONSTRAINT_NAME_SHORT "short_term"

#define ZONE_NAME_PREFIX_PKG "package"
#define ZONE_NAME_CORE "core"
#define ZONE_NAME_UNCORE "uncore"
#define ZONE_NAME_DRAM "dram"
#define ZONE_NAME_PSYS "psys"

static int rapl_open_zone_file(const uint32_t* zones, uint32_t depth, powercap_zone_file type, int flags, int* fd) {
  assert(fd != NULL);
  char buf[PATH_MAX];
  if ((*fd = open_zone_file(CONTROL_TYPE, zones, depth, type, flags)) < 0) {
    // for logging purposes only
    get_zone_file_path(CONTROL_TYPE, zones, depth, type, buf, sizeof(buf));
    if (errno == ENOENT) {
      // No such file or directory
      LOG(DEBUG, "rapl_open_zone_file: access: %s: %s\n", buf, strerror(errno));
      *fd = 0;
    } else if (errno == EACCES && type == POWERCAP_ZONE_FILE_ENERGY_UJ) {
      // special case for energy_uj (it's actually read-only for RAPL)
      errno = 0;
      *fd = open_zone_file(CONTROL_TYPE, zones, depth, type, O_RDONLY);
      if (*fd < 0) {
        LOG(ERROR, "rapl_open_zone_file: open (RO): %s: %s\n", buf, strerror(errno));
      }
    } else {
      LOG(ERROR, "rapl_open_zone_file: open: %s: %s\n", buf, strerror(errno));
    }
  }
  return *fd < 0 ? -errno : 0;
}

static int rapl_open_constraint_file(const uint32_t* zones, uint32_t depth, powercap_constraint_file type, uint32_t constraint, int flags, int* fd) {
  assert(fd != NULL);
  char buf[PATH_MAX];
  if ((*fd = open_constraint_file(CONTROL_TYPE, zones, depth, constraint, type, flags)) < 0) {
    // for logging purposes only
    get_constraint_file_path(CONTROL_TYPE, zones, depth, constraint, type, buf, sizeof(buf));
    if (errno == ENOENT) {
      // No such file or directory
      LOG(DEBUG, "rapl_open_constraint_file: access: %s: %s\n", buf, strerror(errno));
      *fd = 0;
    } else {
      LOG(ERROR, "rapl_open_constraint_file: open: %s: %s\n", buf, strerror(errno));
    }
  }
  return *fd < 0 ? -errno : 0;
}

static int open_zone(const uint32_t* zones, uint32_t depth, powercap_zone* fds, int ro) {
  assert(fds != NULL);
  return rapl_open_zone_file(zones, depth, POWERCAP_ZONE_FILE_MAX_ENERGY_RANGE_UJ, O_RDONLY, &fds->max_energy_range_uj) ||
         rapl_open_zone_file(zones, depth, POWERCAP_ZONE_FILE_ENERGY_UJ, ro ? O_RDONLY : O_RDWR, &fds->energy_uj) ||
         rapl_open_zone_file(zones, depth, POWERCAP_ZONE_FILE_MAX_POWER_RANGE_UW, O_RDONLY, &fds->max_power_range_uw) ||
         rapl_open_zone_file(zones, depth, POWERCAP_ZONE_FILE_POWER_UW, O_RDONLY, &fds->power_uw) ||
         rapl_open_zone_file(zones, depth, POWERCAP_ZONE_FILE_ENABLED, ro ? O_RDONLY : O_RDWR, &fds->enabled) ||
         rapl_open_zone_file(zones, depth, POWERCAP_ZONE_FILE_NAME, O_RDONLY, &fds->name);
}

static int open_constraint(const uint32_t* zones, uint32_t depth, uint32_t constraint, powercap_constraint* fds, int ro) {
  assert(fds != NULL);
  return rapl_open_constraint_file(zones, depth, POWERCAP_CONSTRAINT_FILE_POWER_LIMIT_UW, constraint, ro ? O_RDONLY : O_RDWR, &fds->power_limit_uw) ||
         rapl_open_constraint_file(zones, depth, POWERCAP_CONSTRAINT_FILE_TIME_WINDOW_US, constraint, ro ? O_RDONLY : O_RDWR, &fds->time_window_us) ||
         rapl_open_constraint_file(zones, depth, POWERCAP_CONSTRAINT_FILE_MAX_POWER_UW, constraint, O_RDONLY, &fds->max_power_uw) ||
         rapl_open_constraint_file(zones, depth, POWERCAP_CONSTRAINT_FILE_MIN_POWER_UW, constraint, O_RDONLY, &fds->min_power_uw) ||
         rapl_open_constraint_file(zones, depth, POWERCAP_CONSTRAINT_FILE_MAX_TIME_WINDOW_US, constraint, O_RDONLY, &fds->max_time_window_us) ||
         rapl_open_constraint_file(zones, depth, POWERCAP_CONSTRAINT_FILE_MIN_TIME_WINDOW_US, constraint, O_RDONLY, &fds->min_time_window_us) ||
         rapl_open_constraint_file(zones, depth, POWERCAP_CONSTRAINT_FILE_NAME, constraint, O_RDONLY, &fds->name);
}

static int is_wrong_constraint(const powercap_constraint* fds, const char* expected_name) {
  assert(fds != NULL);
  assert(expected_name != NULL);
  char buf[MAX_NAME_SIZE];
  // assume constraint is wrong unless we can prove it's correct
  return powercap_constraint_get_name(fds, buf, sizeof(buf)) <= 0 ||
         strncmp(buf, expected_name, sizeof(buf)) != 0;
}

static int open_all(const uint32_t* zones, uint32_t depth, powercap_rapl_zone_files* fds, int ro) {
  assert(fds != NULL);
  powercap_constraint tmp;
  if (open_zone(zones, depth, &fds->zone, ro) ||
      open_constraint(zones, depth, CONSTRAINT_NUM_LONG, &fds->constraint_long, ro) ||
      open_constraint(zones, depth, CONSTRAINT_NUM_SHORT, &fds->constraint_short, ro)) {
    return 1;
  }
  // verify that constraints aren't reversed
  // note: never actually seen this problem, but not 100% sure it can't happen, so check anyway...
  if (is_wrong_constraint(&fds->constraint_long, CONSTRAINT_NAME_LONG) &&
      is_wrong_constraint(&fds->constraint_short, CONSTRAINT_NAME_SHORT)) {
    LOG(WARN, "open_all: long and short term constraints are out of order for zone ID: %"PRIu32"\n", zones[0]);
    memcpy(&tmp, &fds->constraint_short, sizeof(powercap_constraint));
    memcpy(&fds->constraint_short, &fds->constraint_long, sizeof(powercap_constraint));
    memcpy(&fds->constraint_long, &tmp, sizeof(powercap_constraint));
  }
  return 0;
}

static const powercap_rapl_zone_files* get_files(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone) {
  assert(pkg != NULL);
  switch (zone) {
    case POWERCAP_RAPL_ZONE_PACKAGE:
      return &pkg->pkg;
    case POWERCAP_RAPL_ZONE_CORE:
      return &pkg->core;
    case POWERCAP_RAPL_ZONE_UNCORE:
      return &pkg->uncore;
    case POWERCAP_RAPL_ZONE_DRAM:
      return &pkg->dram;
    case POWERCAP_RAPL_ZONE_PSYS:
      return &pkg->psys;
    default:
      // somebody passed a bad zone type
      LOG(ERROR, "get_files: Bad powercap_rapl_zone: %d\n", zone);
      errno = EINVAL;
      return NULL;
  }
}

static const powercap_zone* get_zone_files(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone) {
  assert(pkg != NULL);
  const powercap_rapl_zone_files* fds = get_files(pkg, zone);
  return fds == NULL ? NULL : &fds->zone;
}

static const powercap_constraint* get_constraint_files(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint) {
  assert(pkg != NULL);
  const powercap_rapl_zone_files* fds = get_files(pkg, zone);
  if (fds == NULL) {
    return NULL;
  }
  switch (constraint) {
    case POWERCAP_RAPL_CONSTRAINT_LONG:
      return &fds->constraint_long;
    case POWERCAP_RAPL_CONSTRAINT_SHORT:
      return &fds->constraint_short;
    default:
      // somebody passed a bad constraint type
      LOG(ERROR, "get_constraint_files: Bad powercap_rapl_constraint: %d\n", constraint);
      errno = EINVAL;
      return NULL;
  }
}

static int get_zone_fd(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_zone_file file) {
  assert(pkg != NULL);
  const powercap_zone* fds = get_zone_files(pkg, zone);
  if (fds == NULL) {
    return -errno;
  }
  switch (file) {
    case POWERCAP_ZONE_FILE_MAX_ENERGY_RANGE_UJ:
      return fds->max_energy_range_uj;
    case POWERCAP_ZONE_FILE_ENERGY_UJ:
      return fds->energy_uj;
    case POWERCAP_ZONE_FILE_MAX_POWER_RANGE_UW:
      return fds->max_power_range_uw;
    case POWERCAP_ZONE_FILE_POWER_UW:
      return fds->power_uw;
    case POWERCAP_ZONE_FILE_ENABLED:
      return fds->enabled;
    case POWERCAP_ZONE_FILE_NAME:
      return fds->name;
    default:
      LOG(ERROR, "get_zone_fd: Bad powercap_zone_file: %d\n", file);
      errno = EINVAL;
      return -errno;
  }
}

static int get_constraint_fd(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, powercap_constraint_file file) {
  assert(pkg != NULL);
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  if (fds == NULL) {
    return -errno;
  }
  switch (file) {
    case POWERCAP_CONSTRAINT_FILE_POWER_LIMIT_UW:
      return fds->power_limit_uw;
    case POWERCAP_CONSTRAINT_FILE_TIME_WINDOW_US:
      return fds->time_window_us;
    case POWERCAP_CONSTRAINT_FILE_MAX_POWER_UW:
      return fds->max_power_uw;
    case POWERCAP_CONSTRAINT_FILE_MIN_POWER_UW:
      return fds->min_power_uw;
    case POWERCAP_CONSTRAINT_FILE_MAX_TIME_WINDOW_US:
      return fds->max_time_window_us;
    case POWERCAP_CONSTRAINT_FILE_MIN_TIME_WINDOW_US:
      return fds->min_time_window_us;
    case POWERCAP_CONSTRAINT_FILE_NAME:
      return fds->name;
    default:
      LOG(ERROR, "get_constraint_fd: Bad powercap_constraint_file: %d\n", file);
      errno = EINVAL;
      return -errno;
  }
}

static uint32_t get_num_power_planes(uint32_t id) {
  uint32_t zones[2] = { id, 0 };
  while (!powercap_sysfs_zone_exists(CONTROL_TYPE, zones, 2)) {
    zones[1]++;
  }
  return zones[1];
}

static powercap_rapl_zone_files* get_files_by_name(powercap_rapl_pkg* pkg, const uint32_t* zones, uint32_t depth) {
  assert(pkg != NULL);
  char name[MAX_NAME_SIZE];
  if (powercap_sysfs_zone_get_name(CONTROL_TYPE, zones, depth, name, sizeof(name)) < 0) {
    return NULL;
  }
  if (!strncmp(name, ZONE_NAME_PREFIX_PKG, sizeof(ZONE_NAME_PREFIX_PKG) - 1)) {
    return &pkg->pkg;
  } else if (!strncmp(name, ZONE_NAME_CORE, sizeof(ZONE_NAME_CORE))) {
    return &pkg->core;
  } else if (!strncmp(name, ZONE_NAME_UNCORE, sizeof(ZONE_NAME_UNCORE))) {
    return &pkg->uncore;
  } else if (!strncmp(name, ZONE_NAME_DRAM, sizeof(ZONE_NAME_DRAM))) {
    return &pkg->dram;
  } else if (!strncmp(name, ZONE_NAME_PSYS, sizeof(ZONE_NAME_PSYS))) {
    return &pkg->psys;
  } else {
    LOG(ERROR, "get_files_by_name: Unrecognized zone name: %s\n", name);
    errno = EINVAL;
  }
  return NULL;
}

uint32_t powercap_rapl_get_num_packages(void) {
  uint32_t n = 0;
  while (!powercap_sysfs_zone_exists(CONTROL_TYPE, &n, 1)) {
    n++;
  }
  if (!n) {
    LOG(ERROR, "powercap_rapl_get_num_packages: No top-level zones found - is the intel_rapl kernel module loaded?\n");
    errno = ENOENT;
  }
  return n;
}

int powercap_rapl_init(uint32_t id, powercap_rapl_pkg* pkg, int read_only) {
  int ret;
  int err_save;
  uint32_t i;
  uint32_t npp;
  uint32_t zones[2] = { id, 0 };
  powercap_rapl_zone_files *files;
  if (pkg == NULL) {
    errno = EINVAL;
    return -errno;
  }
  // first need the parent zone
  if ((files = get_files_by_name(pkg, zones, 1)) == NULL) {
    return -errno;
  }
  // force all fds to 0 so we don't try to operate on invalid descriptors
  memset(pkg, 0, sizeof(powercap_rapl_pkg));
  // first populate parent zone
  if (!(ret = open_all(zones, 1, files, read_only))) {
    // get subordinate power zones
    npp = get_num_power_planes(id);
    for (i = 0; i < npp && !ret; i++) {
      zones[1] = i;
      if ((files = get_files_by_name(pkg, zones, 2)) == NULL) {
        ret = -errno;
      } else if (files->zone.name) {
        // zone has already been opened ("name" is picked arbitrarily, but it is a required file)
        LOG(ERROR, "powercap_rapl_init: Duplicate zone type detected at %"PRIu32":%"PRIu32"\n", zones[0], zones[1]);
        errno = EBUSY;
        ret = -errno;
      } else {
        ret = open_all(zones, 2, files, read_only);
      }
    }
  }
  if (ret) {
    err_save = errno;
    powercap_rapl_destroy(pkg);
    errno = err_save;
  }
  return ret;
}

static int powercap_rapl_close(int fd) {
  return (fd > 0 && close(fd)) ? -errno : 0;
}

static int fds_destroy_zone(powercap_zone* fds) {
  assert(fds != NULL);
  int ret = 0;
  ret |= powercap_rapl_close(fds->max_energy_range_uj);
  ret |= powercap_rapl_close(fds->energy_uj);
  ret |= powercap_rapl_close(fds->max_power_range_uw);
  ret |= powercap_rapl_close(fds->power_uw);
  ret |= powercap_rapl_close(fds->enabled);
  ret |= powercap_rapl_close(fds->name);
  return ret;
}

static int fds_destroy_zone_constraint(powercap_constraint* fds) {
  assert(fds != NULL);
  int ret = 0;
  ret |= powercap_rapl_close(fds->power_limit_uw);
  ret |= powercap_rapl_close(fds->time_window_us);
  ret |= powercap_rapl_close(fds->max_power_uw);
  ret |= powercap_rapl_close(fds->min_power_uw);
  ret |= powercap_rapl_close(fds->max_time_window_us);
  ret |= powercap_rapl_close(fds->min_time_window_us);
  ret |= powercap_rapl_close(fds->name);
  return ret;
}

static int fds_destroy_all(powercap_rapl_zone_files* files) {
  assert(files != NULL);
  int ret = 0;
  ret |= fds_destroy_zone(&files->zone);
  ret |= fds_destroy_zone_constraint(&files->constraint_long);
  ret |= fds_destroy_zone_constraint(&files->constraint_short);
  return ret;
}

int powercap_rapl_destroy(powercap_rapl_pkg* pkg) {
  int ret = 0;
  if (pkg != NULL) {
    ret |= fds_destroy_all(&pkg->pkg);
    ret |= fds_destroy_all(&pkg->core);
    ret |= fds_destroy_all(&pkg->uncore);
    ret |= fds_destroy_all(&pkg->dram);
    ret |= fds_destroy_all(&pkg->psys);
  }
  return ret;
}

int powercap_rapl_is_zone_supported(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone) {
  // long constraint is always required for zones
  return powercap_rapl_is_constraint_supported(pkg, zone, POWERCAP_RAPL_CONSTRAINT_LONG);
}

int powercap_rapl_is_constraint_supported(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint) {
  // power limit is always required for constraints
  return powercap_rapl_is_constraint_file_supported(pkg, zone, constraint, POWERCAP_CONSTRAINT_FILE_POWER_LIMIT_UW);
}

int powercap_rapl_is_zone_file_supported(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_zone_file file) {
  int fd;
  if (pkg == NULL || (fd = get_zone_fd(pkg, zone, file)) < 0) {
    errno = EINVAL;
    return -errno;
  }
  return fd > 0 ? 1 : 0;
}

int powercap_rapl_is_constraint_file_supported(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, powercap_constraint_file file) {
  int fd;
  if (pkg == NULL || (fd = get_constraint_fd(pkg, zone, constraint, file)) < 0) {
    errno = EINVAL;
    return -errno;
  }
  return fd > 0 ? 1 : 0;
}

ssize_t powercap_rapl_get_name(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, char* buf, size_t size) {
  const powercap_zone* fds = get_zone_files(pkg, zone);
  return fds == NULL ? -errno : powercap_zone_get_name(fds, buf, size);
}

int powercap_rapl_is_enabled(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone) {
  int enabled = -1;
  int ret;
  const powercap_zone* fds = get_zone_files(pkg, zone);
  if (fds == NULL) {
    enabled = -errno;
  } else if ((ret = powercap_zone_get_enabled(fds, &enabled))) {
    enabled = ret;
  }
  return enabled;
}

int powercap_rapl_set_enabled(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, int enabled) {
  const powercap_zone* fds = get_zone_files(pkg, zone);
  return fds == NULL ? -errno : powercap_zone_set_enabled(fds, enabled);
}

int powercap_rapl_get_max_energy_range_uj(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, uint64_t* val) {
  const powercap_zone* fds = get_zone_files(pkg, zone);
  return fds == NULL ? -errno : powercap_zone_get_max_energy_range_uj(fds, val);
}

int powercap_rapl_get_energy_uj(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, uint64_t* val) {
  const powercap_zone* fds = get_zone_files(pkg, zone);
  return fds == NULL ? -errno : powercap_zone_get_energy_uj(fds, val);
}

int powercap_rapl_reset_energy_uj(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone) {
  const powercap_zone* fds = get_zone_files(pkg, zone);
  return fds == NULL ? -errno : powercap_zone_reset_energy_uj(fds);
}

int powercap_rapl_get_max_power_range_uw(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, uint64_t* val) {
  const powercap_zone* fds = get_zone_files(pkg, zone);
  return fds == NULL ? -errno : powercap_zone_get_max_power_range_uw(fds, val);
}

int powercap_rapl_get_power_uw(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, uint64_t* val) {
  const powercap_zone* fds = get_zone_files(pkg, zone);
  return fds == NULL ? -errno : powercap_zone_get_power_uw(fds, val);
}

int powercap_rapl_get_max_power_uw(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, uint64_t* val) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -errno : powercap_constraint_get_max_power_uw(fds, val);
}

int powercap_rapl_get_min_power_uw(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, uint64_t* val) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -errno : powercap_constraint_get_min_power_uw(fds, val);
}

int powercap_rapl_get_power_limit_uw(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, uint64_t* val) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -errno : powercap_constraint_get_power_limit_uw(fds, val);
}

int powercap_rapl_set_power_limit_uw(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, uint64_t val) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -errno : powercap_constraint_set_power_limit_uw(fds, val);
}

int powercap_rapl_get_max_time_window_us(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, uint64_t* val) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -errno : powercap_constraint_get_max_time_window_us(fds, val);
}

int powercap_rapl_get_min_time_window_us(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, uint64_t* val) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -errno : powercap_constraint_get_min_time_window_us(fds, val);
}

int powercap_rapl_get_time_window_us(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, uint64_t* val) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -errno : powercap_constraint_get_time_window_us(fds, val);
}

int powercap_rapl_set_time_window_us(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, uint64_t val) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -errno : powercap_constraint_set_time_window_us(fds, val);
}

ssize_t powercap_rapl_get_constraint_name(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, char* buf, size_t size) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -errno : powercap_constraint_get_name(fds, buf, size);
}
