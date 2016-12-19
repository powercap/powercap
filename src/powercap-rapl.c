/**
 * RAPL implementation of powercap.
 *
 * @author Connor Imes
 * @date 2016-05-12
 */
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "powercap-rapl.h"
#include "powercap.h"

#define POWERCAP_BASEDIR "/sys/class/powercap"
#define RAPL_PREFIX "intel-rapl:"

#define CONSTRAINT_NUM_LONG 0
#define CONSTRAINT_NUM_SHORT 1

#define CONSTRAINT_NAME_LONG "long_term"
#define CONSTRAINT_NAME_SHORT "short_term"

#define PP_NAME_CORE "core"
#define PP_NAME_UNCORE "uncore"
#define PP_NAME_DRAM "dram"
#define PP_NAME_PSYS "psys"

static int open_zone_file(uint32_t pkg, uint32_t pp, int is_pp, powercap_zone_file type, int flags, int* fd) {
  assert(fd != NULL);
  char buf[128];
  // first get the directory
  if (is_pp) {
    // a power plane - subdirectory of package
    snprintf(buf, sizeof(buf), POWERCAP_BASEDIR"/intel-rapl:%"PRIu32"/intel-rapl:%"PRIu32":%"PRIu32"/", pkg, pkg, pp);
  } else {
    // package-level only
    snprintf(buf, sizeof(buf), POWERCAP_BASEDIR"/intel-rapl:%"PRIu32"/", pkg);
  }
  size_t len = strlen(buf);
  // now append the filename based on the type
  powercap_zone_file_get_name(type, buf + len, sizeof(buf) - len);
  // only try to open if file exists, otherwise set fd to 0
  if (access(buf, F_OK) != -1) {
    *fd = open(buf, flags);
    // special case for energy_uj (it's sometimes read-only):
    // TODO: Using "access(buf, W_OK) == 0" as superuser seems to always be true, so we had to try to open instead
    if (*fd < 0 && errno == EACCES && type == POWERCAP_ZONE_FILE_ENERGY_UJ) {
      errno = 0;
      *fd = open(buf, O_RDONLY);
    }
  } else {
    *fd = 0;
  }
  return *fd < 0;
}

static int open_constraint_file(uint32_t pkg, uint32_t pp, int is_pp, powercap_constraint_file type, uint32_t constraint, int flags, int* fd) {
  assert(fd != NULL);
  char buf[128];
  // first get the directory
  if (is_pp) {
    // a power plane - subdirectory of package
    snprintf(buf, sizeof(buf), POWERCAP_BASEDIR"/intel-rapl:%"PRIu32"/intel-rapl:%"PRIu32":%"PRIu32"/", pkg, pkg, pp);
  } else {
    // package-level only
    snprintf(buf, sizeof(buf), POWERCAP_BASEDIR"/intel-rapl:%"PRIu32"/", pkg);
  }
  // now append the filename based on the type and constraint
  size_t len = strlen(buf);
  powercap_constraint_file_get_name(type, constraint, buf + len, sizeof(buf) - len);
  // only try to open if file exists, otherwise set fd to 0
  if (access(buf, F_OK) != -1) {
    *fd = open(buf, flags);
  } else {
    *fd = 0;
  }
  return *fd < 0;
}

static int open_zone(uint32_t pkg, uint32_t pp, int is_pp, powercap_zone* fds, int ro) {
  assert(fds != NULL);
  return open_zone_file(pkg, pp, is_pp, POWERCAP_ZONE_FILE_MAX_ENERGY_RANGE_UJ, O_RDONLY, &fds->max_energy_range_uj) ||
         open_zone_file(pkg, pp, is_pp, POWERCAP_ZONE_FILE_ENERGY_UJ, ro ? O_RDONLY : O_RDWR, &fds->energy_uj) ||
         open_zone_file(pkg, pp, is_pp, POWERCAP_ZONE_FILE_MAX_POWER_RANGE_UW, O_RDONLY, &fds->max_power_range_uw) ||
         open_zone_file(pkg, pp, is_pp, POWERCAP_ZONE_FILE_POWER_UW, O_RDONLY, &fds->power_uw) ||
         open_zone_file(pkg, pp, is_pp, POWERCAP_ZONE_FILE_ENABLED, ro ? O_RDONLY : O_RDWR, &fds->enabled) ||
         open_zone_file(pkg, pp, is_pp, POWERCAP_ZONE_FILE_NAME, O_RDONLY, &fds->name);
}

static int open_constraint(uint32_t pkg, uint32_t pp, int is_pp, uint32_t constraint, powercap_constraint* fds, int ro) {
  assert(fds != NULL);
  return open_constraint_file(pkg, pp, is_pp, POWERCAP_CONSTRAINT_FILE_POWER_LIMIT_UW, constraint, ro ? O_RDONLY : O_RDWR, &fds->power_limit_uw) ||
         open_constraint_file(pkg, pp, is_pp, POWERCAP_CONSTRAINT_FILE_TIME_WINDOW_US, constraint, ro ? O_RDONLY : O_RDWR, &fds->time_window_us) ||
         open_constraint_file(pkg, pp, is_pp, POWERCAP_CONSTRAINT_FILE_MAX_POWER_UW, constraint, O_RDONLY, &fds->max_power_uw) ||
         open_constraint_file(pkg, pp, is_pp, POWERCAP_CONSTRAINT_FILE_MIN_POWER_UW, constraint, O_RDONLY, &fds->min_power_uw) ||
         open_constraint_file(pkg, pp, is_pp, POWERCAP_CONSTRAINT_FILE_MAX_TIME_WINDOW_US, constraint, O_RDONLY, &fds->max_time_window_us) ||
         open_constraint_file(pkg, pp, is_pp, POWERCAP_CONSTRAINT_FILE_MIN_TIME_WINDOW_US, constraint, O_RDONLY, &fds->min_time_window_us) ||
         open_constraint_file(pkg, pp, is_pp, POWERCAP_CONSTRAINT_FILE_NAME, constraint, O_RDONLY, &fds->name);
}

static int is_wrong_constraint(const powercap_constraint* fds, const char* expected_name) {
  assert(fds != NULL);
  assert(expected_name != NULL);
  char buf[32];
  // assume constraint is wrong unless we can prove it's correct
  return powercap_constraint_get_name(fds, buf, sizeof(buf)) <= 0 ||
         strncmp(buf, expected_name, sizeof(buf)) != 0;
}

static int open_all(uint32_t pkg, uint32_t pp, int is_pp, powercap_rapl_zone_files* fds, int ro) {
  assert(fds != NULL);
  powercap_constraint tmp;
  if (open_zone(pkg, pp, is_pp, &fds->zone, ro) ||
      open_constraint(pkg, pp, is_pp, CONSTRAINT_NUM_LONG, &fds->constraint_long, ro) ||
      open_constraint(pkg, pp, is_pp, CONSTRAINT_NUM_SHORT, &fds->constraint_short, ro)) {
    return 1;
  }
  // verify that constraints aren't reversed
  // note: never actually seen this problem, but not 100% sure it can't happen, so check anyway...
  if (is_wrong_constraint(&fds->constraint_long, CONSTRAINT_NAME_LONG) &&
      is_wrong_constraint(&fds->constraint_short, CONSTRAINT_NAME_SHORT)) {
    // fprintf(stderr, "Warning: long and short term constraints are out of order for pkg %"PRIu32"\n", pkg);
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
      errno = EINVAL;
      return NULL;
  }
}

static inline int get_zone_fd(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_zone_file file) {
  assert(pkg != NULL);
  const powercap_zone* fds = get_zone_files(pkg, zone);
  if (fds == NULL) {
    return -1;
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
      errno = EINVAL;
      return -1;
  }
}

static inline int get_constraint_fd(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, powercap_constraint_file file) {
  assert(pkg != NULL);
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  if (fds == NULL) {
    return -1;
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
      errno = EINVAL;
      return -1;
  }
}

static inline uint32_t get_num_power_planes(uint32_t pkg) {
  // scan the sysfs directory
  uint32_t count = 0;
  int err_save;
  struct dirent* entry;
  char buf[64];
  snprintf(buf, sizeof(buf), POWERCAP_BASEDIR"/intel-rapl:%"PRIu32, pkg);
  DIR* dir = opendir(buf);
  for (errno = 0; dir != NULL && (entry = readdir(dir)) != NULL;) {
    // no order guarantee from readdir, just count 'intel-rapl:pkg:#' entries
    // strlen(buf) really is what we want, rather than sizeof(buf), so we ignore the trailing numerals in entry->d_name
    snprintf(buf, sizeof(buf), "intel-rapl:%"PRIu32":", pkg);
    count += strncmp(entry->d_name, buf, strlen(buf)) ? 0 : 1;
  }
  err_save = errno; // from opendir or readdir
  if (dir != NULL) {
    closedir(dir);
  }
  errno = err_save;
  return errno ? 0 : count;
}

static inline int get_pp_type(uint32_t pkg, uint32_t pp, powercap_rapl_zone* zone) {
  assert(zone != NULL);
  char buf[128];
  char name[8];
  int ret = -1;
  snprintf(buf, sizeof(buf), POWERCAP_BASEDIR"/intel-rapl:%"PRIu32"/intel-rapl:%"PRIu32":%"PRIu32"/name", pkg, pkg, pp);
  FILE* f = fopen(buf, "r");
  if (f != NULL) {
    if (fgets(name, sizeof(name), f) != NULL) {
      // we use "sizeof(...)-1" as max number of chars to compare so we ignore newline characters in the name buffer
      if (!strncmp(name, PP_NAME_CORE, sizeof(PP_NAME_CORE) - 1)) {
        *zone = POWERCAP_RAPL_ZONE_CORE;
        ret = 0;
      } else if (!strncmp(name, PP_NAME_UNCORE, sizeof(PP_NAME_UNCORE) - 1)) {
        *zone = POWERCAP_RAPL_ZONE_UNCORE;
        ret = 0;
      } else if (!strncmp(name, PP_NAME_DRAM, sizeof(PP_NAME_DRAM) - 1)) {
        *zone = POWERCAP_RAPL_ZONE_DRAM;
        ret = 0;
      } else if (!strncmp(name, PP_NAME_PSYS, sizeof(PP_NAME_PSYS) - 1)) {
        *zone = POWERCAP_RAPL_ZONE_PSYS;
        ret = 0;
      }
    }
    fclose(f);
  }
  return ret;
}

uint32_t powercap_rapl_get_num_packages(void) {
  // scan the sysfs directory
  uint32_t count = 0;
  int err_save;
  struct dirent* entry;
  DIR* dir = opendir(POWERCAP_BASEDIR);
  if (dir != NULL) {
    for (errno = 0; (entry = readdir(dir)) != NULL;) {
      // no order guarantee from readdir, so just count directories of the form "intel-rapl:#", but not "intel-rapl:#:#"
      if (strncmp(entry->d_name, RAPL_PREFIX, sizeof(RAPL_PREFIX) - 1) == 0 &&
          strchr(entry->d_name + sizeof(RAPL_PREFIX) - 1, ':') == NULL) {
        count++;
      }
    }
    err_save = errno; // from opendir or readdir
    closedir(dir);
    errno = err_save;
  }
  return errno ? 0 : count;
}

int powercap_rapl_init(uint32_t package, powercap_rapl_pkg* pkg, int read_only) {
  int ret = 0;
  int err_save;
  uint32_t i;
  uint32_t npp;
  powercap_rapl_zone type;
  if (pkg == NULL) {
    errno = EINVAL;
    return -1;
  }
  // force all fds to 0 so we don't try to operate on invalid descriptors
  memset(pkg, 0, sizeof(powercap_rapl_pkg));
  // first populate zone and package power zone
  if (open_all(package, 0, 0, &pkg->pkg, read_only)) {
    ret = -1;
  } else {
    // get a count of subordinate power zones in this package
    npp = get_num_power_planes(package);
    // now get all power zones
    for (i = 0; i < npp && !ret; i++) {
      ret = get_pp_type(package, i, &type);
      if (ret) {
        break;
      }
      switch (type) {
        case POWERCAP_RAPL_ZONE_CORE:
          ret = open_all(package, i, 1, &pkg->core, read_only);
          break;
        case POWERCAP_RAPL_ZONE_UNCORE:
          ret = open_all(package, i, 1, &pkg->uncore, read_only);
          break;
        case POWERCAP_RAPL_ZONE_DRAM:
          ret = open_all(package, i, 1, &pkg->dram, read_only);
          break;
        case POWERCAP_RAPL_ZONE_PSYS:
          ret = open_all(package, i, 1, &pkg->psys, read_only);
          break;
        case POWERCAP_RAPL_ZONE_PACKAGE:
        default:
          assert(0);
          break;
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

static inline int powercap_rapl_close(int fd) {
  return fd > 0 ? close(fd) : 0;
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
    return -1;
  }
  return fd > 0 ? 1 : 0;
}

int powercap_rapl_is_constraint_file_supported(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, powercap_constraint_file file) {
  int fd;
  if (pkg == NULL || (fd = get_constraint_fd(pkg, zone, constraint, file)) < 0) {
    errno = EINVAL;
    return -1;
  }
  return fd > 0 ? 1 : 0;
}

ssize_t powercap_rapl_get_name(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, char* buf, size_t size) {
  const powercap_zone* fds = get_zone_files(pkg, zone);
  return fds == NULL ? -1 : powercap_zone_get_name(fds, buf, size);
}

int powercap_rapl_is_enabled(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone) {
  int enabled = -1;
  const powercap_zone* fds = get_zone_files(pkg, zone);
  if (fds != NULL) {
    powercap_zone_get_enabled(fds, &enabled);
  }
  return enabled;
}

int powercap_rapl_set_enabled(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, int enabled) {
  const powercap_zone* fds = get_zone_files(pkg, zone);
  return fds == NULL ? -1 : powercap_zone_set_enabled(fds, enabled);
}

int powercap_rapl_get_max_energy_range_uj(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, uint64_t* val) {
  const powercap_zone* fds = get_zone_files(pkg, zone);
  return fds == NULL ? -1 : powercap_zone_get_max_energy_range_uj(fds, val);
}

int powercap_rapl_get_energy_uj(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, uint64_t* val) {
  const powercap_zone* fds = get_zone_files(pkg, zone);
  return fds == NULL ? -1 : powercap_zone_get_energy_uj(fds, val);
}

int powercap_rapl_reset_energy_uj(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone) {
  const powercap_zone* fds = get_zone_files(pkg, zone);
  return fds == NULL ? -1 : powercap_zone_reset_energy_uj(fds);
}

int powercap_rapl_get_max_power_range_uw(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, uint64_t* val) {
  const powercap_zone* fds = get_zone_files(pkg, zone);
  return fds == NULL ? -1 : powercap_zone_get_max_power_range_uw(fds, val);
}

int powercap_rapl_get_power_uw(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, uint64_t* val) {
  const powercap_zone* fds = get_zone_files(pkg, zone);
  return fds == NULL ? -1 : powercap_zone_get_power_uw(fds, val);
}

int powercap_rapl_get_max_power_uw(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, uint64_t* val) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -1 : powercap_constraint_get_max_power_uw(fds, val);
}

int powercap_rapl_get_min_power_uw(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, uint64_t* val) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -1 : powercap_constraint_get_min_power_uw(fds, val);
}

int powercap_rapl_get_power_limit_uw(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, uint64_t* val) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -1 : powercap_constraint_get_power_limit_uw(fds, val);
}

int powercap_rapl_set_power_limit_uw(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, uint64_t val) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -1 : powercap_constraint_set_power_limit_uw(fds, val);
}

int powercap_rapl_get_max_time_window_us(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, uint64_t* val) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -1 : powercap_constraint_get_max_time_window_us(fds, val);
}

int powercap_rapl_get_min_time_window_us(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, uint64_t* val) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -1 : powercap_constraint_get_min_time_window_us(fds, val);
}

int powercap_rapl_get_time_window_us(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, uint64_t* val) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -1 : powercap_constraint_get_time_window_us(fds, val);
}

int powercap_rapl_set_time_window_us(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, uint64_t val) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -1 : powercap_constraint_set_time_window_us(fds, val);
}

ssize_t powercap_rapl_get_constraint_name(const powercap_rapl_pkg* pkg, powercap_rapl_zone zone, powercap_rapl_constraint constraint, char* buf, size_t size) {
  const powercap_constraint* fds = get_constraint_files(pkg, zone, constraint);
  return fds == NULL ? -1 : powercap_constraint_get_name(fds, buf, size);
}
