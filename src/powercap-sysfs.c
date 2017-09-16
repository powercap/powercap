/**
 * Read/write powercap sysfs files.
 *
 * @author Connor Imes
 * @date 2017-08-24
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "powercap-sysfs.h"

/* Zone files */
enum {
  ZONE_MAX_ENERGY_RANGE_UJ,
  ZONE_ENERGY_UJ,
  ZONE_MAX_POWER_RANGE_UW,
  ZONE_POWER_UW,
  ZONE_ENABLED,
  ZONE_NAME,
  ZONE_FILE_COUNT
};

static const char* ZONE_FILE[ZONE_FILE_COUNT] = {
  "max_energy_range_uj",
  "energy_uj",
  "max_power_range_uw",
  "power_uw",
  "enabled",
  "name"
};

/* Constraint files */
enum {
  CONSTRAINT_POWER_LIMIT_UW,
  CONSTRAINT_TIME_WINDOW_US,
  CONSTRAINT_MAX_POWER_UW,
  CONSTRAINT_MIN_POWER_UW,
  CONSTRAINT_MAX_TIME_WINDOW_US,
  CONSTRAINT_MIN_TIME_WINDOW_US,
  CONSTRAINT_NAME,
  CONSTRAINT_FILE_COUNT
};

static const char* CONSTRAINT_FILE_SUFFIX[CONSTRAINT_FILE_COUNT] = {
  "power_limit_uw",
  "time_window_us",
  "max_power_uw",
  "min_power_uw",
  "max_time_window_us",
  "min_time_window_us",
  "name"
};

#ifdef USE_VIRTUAL_DEVICES
  #define POWERCAP_PATH "/sys/devices/virtual/powercap"
#else
  #define POWERCAP_PATH "/sys/class/powercap"
#endif
/* PATH_MAX should be defined in limits.h */
#ifndef PATH_MAX
  #pragma message("Warning: PATH_MAX was not defined")
  #define PATH_MAX 4096
#endif
#define MAX_U64_SIZE 24

/* buf must not be NULL and size >= 1 */
static ssize_t read_string_safe(int fd, char* buf, size_t size) {
  ssize_t ret;
  if ((ret = read(fd, buf, size - 1)) > 0) {
    /* force a terminating character in the buffer */
    if (buf[ret - 1] == '\n') {
      /* also remove newline character */
      buf[ret - 1] = '\0';
    } else {
      buf[ret] = '\0';
    }
  } else if (ret < 0) {
    ret = -errno;
  } else {
    errno = ENODATA;
    ret = -errno;
  }
  return ret;
}

static ssize_t read_string(int fd, char* buf, size_t size) {
  if (!buf) {
    errno = EINVAL;
    return (ssize_t) -errno;
  } else if (!size) {
    errno = ENOBUFS;
    return (ssize_t) -errno;
  }
  return read_string_safe(fd, buf, size);
}

static int read_u64(int fd, uint64_t* val) {
  char buf[MAX_U64_SIZE];
  char* end;
  if (!val) {
    errno = EINVAL;
  } else if (read_string_safe(fd, buf, sizeof(buf)) > 0) {
    errno = 0;
    *val = strtoull(buf, &end, 0);
    if (buf != end && errno != ERANGE) {
      return 0;
    }
  }
  return -errno;
}

static int write_u64(int fd, uint64_t val) {
  char buf[MAX_U64_SIZE];
  ssize_t written;
  snprintf(buf, sizeof(buf), "%"PRIu64, val);
  if ((written = write(fd, buf, sizeof(buf))) < 0) {
    return -errno;
  }
  if (!written) {
    /* Is there a better error code? */
    errno = EIO;
    return -errno;
  }
  return 0;
}

/* Returns 0 on failure like insufficient buffer size */
static size_t append_zone_dir(const char* control_type, const uint32_t* zones, uint32_t depth, char* path, size_t size) {
  size_t written;
  int n;
  uint32_t i;
  n = snprintf(path, size, "/%s", control_type);
  if (n <= 0 || (size_t) n >= size) {
    errno = ENOBUFS;
    return 0;
  }
  written = (size_t) n;
  for (i = 0; i < depth; i++) {
    n = snprintf(path + written, size - written, ":%"PRIu32, zones[i]);
    if (n <= 0 || (size_t) n >= size - written) {
      errno = ENOBUFS;
      return 0;
    }
    written += (size_t) n;
  }
  return written;
}

/*
 * Returns 0 on failure like insufficient buffer size or if control_type is NULL.
 * zones can be NULL only if depth is 0; path must not be NULL.
 */
static size_t get_base_path(const char* control_type, const uint32_t* zones, uint32_t depth, char* path, size_t size) {
  size_t written;
  int n;
  uint32_t i;
  size_t tmp;
  /* simple names only, trying to look outside the powercap directory is not allowed */
  if (!control_type || !strlen(control_type) || strcspn(control_type, "./") != strlen(control_type)) {
    errno = EINVAL;
    return 0;
  }
  n = snprintf(path, size, POWERCAP_PATH"/%s", control_type);
  if (n <= 0 || (size_t) n >= size) {
    errno = ENOBUFS;
    return 0;
  }
  written = (size_t) n;
  for (i = 1; i <= depth; i++) {
    if (!zones) {
      errno = EINVAL;
      return 0;
    }
    if (!(tmp = append_zone_dir(control_type, zones, i, path + written, size - written))) {
      return 0;
    }
    written += tmp;
  }
  return written;
}

/* Returns 0 on failure like insufficient buffer size */
static size_t get_zone_file_path(const char* control_type, const uint32_t* zones, uint32_t depth, unsigned int which,
                                 char* path, size_t size) {
  size_t written;
  int n;
  if ((written = get_base_path(control_type, zones, depth, path, size)) > 0) {
    n = snprintf(path + written, size - written, "/%s", ZONE_FILE[which]);
    if (n <= 0 || (size_t) n >= size - written) {
      errno = ENOBUFS;
      return 0;
    }
    written += (size_t) n;
  }
  return written;
}

/* Returns 0 on failure like insufficient buffer size */
static size_t get_constraint_file_path(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint,
                                       unsigned int which, char* path, size_t size) {
  size_t written;
  int n;
  written = get_base_path(control_type, zones, depth, path, size);
  if (written > 0) {
    n = snprintf(path + written, size - written, "/constraint_%"PRIu32"_%s", constraint, CONSTRAINT_FILE_SUFFIX[which]);
    if (n <= 0 || (size_t) n >= size - written) {
      errno = ENOBUFS;
      return 0;
    }
    written += (size_t) n;
  }
  return written;
}

static int open_zone_file(const char* control_type, const uint32_t* zones, uint32_t depth, unsigned int which, int flags) {
  char path[PATH_MAX];
  if (!get_zone_file_path(control_type, zones, depth, which, path, sizeof(path))) {
    return -errno;
  }
  return open(path, flags);
}

static int open_constraint_file(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint,
                                unsigned int which, int flags) {
  char path[PATH_MAX];
  if (!get_constraint_file_path(control_type, zones, depth, constraint, which, path, sizeof(path))) {
    return -errno;
  }
  return open(path, flags);
}

static int zone_read_u64(const char* control_type, const uint32_t* zones, uint32_t depth, uint64_t* val,
                         unsigned int which) {
  int ret;
  int fd;
  if ((fd = open_zone_file(control_type, zones, depth, which, O_RDONLY)) < 0) {
    return -errno;
  }
  ret = read_u64(fd, val);
  close(fd);
  return ret;
}

static int constraint_read_u64(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint,
                               uint64_t* val, unsigned int which) {
  int ret;
  int fd;
  if ((fd = open_constraint_file(control_type, zones, depth, constraint, which, O_RDONLY)) < 0) {
    return -errno;
  }
  ret = read_u64(fd, val);
  close(fd);
  return ret;
}

static int constraint_write_u64(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint,
                                uint64_t val, unsigned int which) {
  int ret;
  int fd;
  if ((fd = open_constraint_file(control_type, zones, depth, constraint, which, O_WRONLY)) < 0) {
    return -errno;
  }
  ret = write_u64(fd, val);
  close(fd);
  return ret;
}

int powercap_sysfs_control_type_exists(const char* control_type) {
  return powercap_sysfs_zone_exists(control_type, NULL, 0);
}

int powercap_sysfs_zone_exists(const char* control_type, const uint32_t* zones, uint32_t depth) {
  char path[PATH_MAX];
  struct stat ss;
  if (!get_base_path(control_type, zones, depth, path, sizeof(path))) {
    return -errno;
  }
  if (stat(path, &ss) || !S_ISDIR(ss.st_mode)) {
    errno = ENOSYS;
    return -errno;
  }
  return 0;
}

int powercap_sysfs_constraint_exists(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint) {
  char path[PATH_MAX];
  struct stat ss;
  /* power_limit_uw file must exist */
  if (!get_constraint_file_path(control_type, zones, depth, constraint, CONSTRAINT_POWER_LIMIT_UW, path, sizeof(path))) {
    return -errno;
  }
  if (stat(path, &ss) || !S_ISREG(ss.st_mode)) {
    errno = ENOSYS;
    return -errno;
  }
  return 0;
}

int powercap_sysfs_zone_get_max_energy_range_uj(const char* control_type, const uint32_t* zones, uint32_t depth, uint64_t* val) {
  return zone_read_u64(control_type, zones, depth, val, ZONE_MAX_ENERGY_RANGE_UJ);
}

int powercap_sysfs_zone_set_energy_uj(const char* control_type, const uint32_t* zones, uint32_t depth, uint64_t val) {
  int ret;
  int fd;
  if ((fd = open_zone_file(control_type, zones, depth, ZONE_ENERGY_UJ, O_WRONLY)) < 0) {
    return -errno;
  }
  ret = write_u64(fd, val);
  close(fd);
  return ret;
}

int powercap_sysfs_zone_get_energy_uj(const char* control_type, const uint32_t* zones, uint32_t depth, uint64_t* val) {
  return zone_read_u64(control_type, zones, depth, val, ZONE_ENERGY_UJ);
}

int powercap_sysfs_zone_get_max_power_range_uw(const char* control_type, const uint32_t* zones, uint32_t depth, uint64_t* val) {
  return zone_read_u64(control_type, zones, depth, val, ZONE_MAX_POWER_RANGE_UW);
}

int powercap_sysfs_zone_get_power_uw(const char* control_type, const uint32_t* zones, uint32_t depth, uint64_t* val) {
  return zone_read_u64(control_type, zones, depth, val, ZONE_POWER_UW);
}

int powercap_sysfs_zone_set_enabled(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t val) {
  int ret;
  int fd;
  if ((fd = open_zone_file(control_type, zones, depth, ZONE_ENABLED, O_WRONLY)) < 0) {
    return -errno;
  }
  ret = write_u64(fd, (uint64_t) val);
  close(fd);
  return ret;
}

int powercap_sysfs_zone_get_enabled(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t* val) {
  uint64_t enabled;
  int ret;
  if (val) {
    if (!(ret = zone_read_u64(control_type, zones, depth, &enabled, ZONE_ENABLED))) {
      *val = (uint32_t) enabled;
    }
  } else {
    errno = EINVAL;
    ret = -errno;
  }
  return ret;
}

ssize_t powercap_sysfs_zone_get_name(const char* control_type, const uint32_t* zones, uint32_t depth, char* buf, size_t size) {
  ssize_t ret;
  int fd;
  if ((fd = open_zone_file(control_type, zones, depth, ZONE_NAME, O_RDONLY)) < 0) {
    return -errno;
  }
  ret = read_string(fd, buf, size);
  close(fd);
  return ret;
}

int powercap_sysfs_constraint_set_power_limit_uw(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint, uint64_t val) {
  return constraint_write_u64(control_type, zones, depth, constraint, val, CONSTRAINT_POWER_LIMIT_UW);
}

int powercap_sysfs_constraint_get_power_limit_uw(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint, uint64_t* val) {
  return constraint_read_u64(control_type, zones, depth, constraint, val, CONSTRAINT_POWER_LIMIT_UW);
}

int powercap_sysfs_constraint_set_time_window_us(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint, uint64_t val) {
  return constraint_write_u64(control_type, zones, depth, constraint, val, CONSTRAINT_TIME_WINDOW_US);
}

int powercap_sysfs_constraint_get_time_window_us(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint, uint64_t* val) {
  return constraint_read_u64(control_type, zones, depth, constraint, val, CONSTRAINT_TIME_WINDOW_US);
}

int powercap_sysfs_constraint_get_max_power_uw(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint, uint64_t* val) {
  return constraint_read_u64(control_type, zones, depth, constraint, val, CONSTRAINT_MAX_POWER_UW);
}

int powercap_sysfs_constraint_get_min_power_uw(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint, uint64_t* val) {
  return constraint_read_u64(control_type, zones, depth, constraint, val, CONSTRAINT_MIN_POWER_UW);
}

int powercap_sysfs_constraint_get_max_time_window_us(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint, uint64_t* val) {
  return constraint_read_u64(control_type, zones, depth, constraint, val, CONSTRAINT_MAX_TIME_WINDOW_US);
}

int powercap_sysfs_constraint_get_min_time_window_us(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint, uint64_t* val) {
  return constraint_read_u64(control_type, zones, depth, constraint, val, CONSTRAINT_MIN_TIME_WINDOW_US);
}

ssize_t powercap_sysfs_constraint_get_name(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint, char* buf, size_t size) {
  ssize_t ret;
  int fd;
  if ((fd = open_constraint_file(control_type, zones, depth, constraint, CONSTRAINT_NAME, O_RDONLY)) < 0) {
    return -errno;
  }
  ret = read_string(fd, buf, size);
  close(fd);
  return ret;
}
