/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Common functions.
 *
 * @author Connor Imes
 * @date 2017-08-24
 */
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
/* Main powercap header only used for enums */
#include "powercap.h"
#include "powercap-common.h"

#define MAX_U64_SIZE 24

#ifdef USE_VIRTUAL_DEVICES
  #define POWERCAP_PATH "/sys/devices/virtual/powercap"
#else
  #define POWERCAP_PATH "/sys/class/powercap"
#endif

/* These enums MUST align with powercap_control_type_file in powercap.h */
static const char* CONTROL_TYPE_FILE[] = {
  "enabled"
};

/* These enums MUST align with powercap_zone_file in powercap.h */
static const char* ZONE_FILE[] = {
  "max_energy_range_uj",
  "energy_uj",
  "max_power_range_uw",
  "power_uw",
  "enabled",
  "name"
};

/* These enums MUST align with powercap_constraint_file in powercap.h */
static const char* CONSTRAINT_FILE_SUFFIX[] = {
  "power_limit_uw",
  "time_window_us",
  "max_power_uw",
  "min_power_uw",
  "max_time_window_us",
  "min_time_window_us",
  "name"
};

ssize_t read_string_safe(int fd, char* buf, size_t size) {
  ssize_t ret;
  if ((ret = pread(fd, buf, size - 1, 0)) > 0) {
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

ssize_t read_string(int fd, char* buf, size_t size) {
  if (!buf) {
    errno = EINVAL;
    return (ssize_t) -errno;
  } else if (!size) {
    errno = ENOBUFS;
    return (ssize_t) -errno;
  }
  return read_string_safe(fd, buf, size);
}

int read_u64(int fd, uint64_t* val) {
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

int write_u64(int fd, uint64_t val) {
  char buf[MAX_U64_SIZE];
  ssize_t written;
  snprintf(buf, sizeof(buf), "%"PRIu64, val);
  if ((written = pwrite(fd, buf, sizeof(buf), 0)) < 0) {
    return -errno;
  }
  if (!written) {
    /* Is there a better error code? */
    errno = EIO;
    return -errno;
  }
  return 0;
}

int control_type_file_get_name(powercap_control_type_file type, char* buf, size_t size) {
  /* check type in case users pass bad int value instead of enum; int cast silences clang compiler */
  if (!buf || !size || (int) type < 0 || (int) type > POWERCAP_CONTROL_TYPE_FILE_ENABLED) {
    errno = EINVAL;
    return -errno;
  }
  return snprintf(buf, size, "%s", CONTROL_TYPE_FILE[type]);
}

int zone_file_get_name(powercap_zone_file type, char* buf, size_t size) {
  /* check type in case users pass bad int value instead of enum; int cast silences clang compiler */
  if (!buf || !size || (int) type < 0 || (int) type > POWERCAP_ZONE_FILE_NAME) {
    errno = EINVAL;
    return -errno;
  }
  return snprintf(buf, size, "%s", ZONE_FILE[type]);
}

int constraint_file_get_name(powercap_constraint_file type, uint32_t constraint, char* buf, size_t size) {
  /* check type in case users pass bad int value instead of enum; int cast silences clang compiler */
  if (!buf || !size || (int) type < 0 || (int) type > POWERCAP_CONSTRAINT_FILE_NAME) {
    errno = EINVAL;
    return -errno;
  }
  return snprintf(buf, size, "constraint_%"PRIu32"_%s", constraint, CONSTRAINT_FILE_SUFFIX[type]);
}

/* Returns 0 on error or if no more buffer space, with errno set */
static size_t snprintf_ret_to_size_t(int ret, size_t max_size) {
  if (ret < 0) {
    /* shouldn't happen */
    if (!errno) {
      errno = EINVAL;
    }
    return 0;
  }
  if (ret == 0 || (size_t) ret >= max_size) {
    errno = ENOBUFS;
    return 0;
  }
  return (size_t) ret;
}

/* Returns 0 on failure like insufficient buffer size */
static size_t append_zone_dir(const char* control_type, const uint32_t* zones, uint32_t depth, char* path, size_t size) {
  size_t written;
  size_t n;
  uint32_t i;
  if ((written = snprintf_ret_to_size_t(snprintf(path, size - 1, "%s", control_type), size - 1))) {
    for (i = 0; i < depth; i++) {
      if (!(n = snprintf_ret_to_size_t(snprintf(path + written, size - written - 1, ":%x", zones[i]), size - written - 1))) {
        return 0;
      }
      written += n;
    }
    path[written++] = '/';
    path[written] = '\0';
  }
  return written;
}

size_t get_base_path(const char* control_type, const uint32_t* zones, uint32_t depth, char* path, size_t size) {
  size_t written;
  size_t n;
  uint32_t i;
  /* simple names only, trying to look outside the powercap directory is not allowed */
  if (!control_type || !strlen(control_type) || strcspn(control_type, "./") != strlen(control_type) || (depth && !zones)) {
    errno = EINVAL;
    return 0;
  }
  if ((written = snprintf_ret_to_size_t(snprintf(path, size, POWERCAP_PATH"/%s/", control_type), size))) {
    for (i = 1; i <= depth; i++) {
      if (!(n = append_zone_dir(control_type, zones, i, path + written, size - written))) {
        return 0;
      }
      written += n;
    }
  }
  return written;
}

size_t get_control_type_file_path(const char* control_type, powercap_control_type_file type, char* path, size_t size) {
  size_t written;
  int n;
  if ((written = get_base_path(control_type, NULL, 0, path, size))) {
    n = control_type_file_get_name(type, path + written, size - written);
    if (!snprintf_ret_to_size_t(n, size - written)) {
      return 0;
    }
    written += (size_t) n;
  }
  return written;
}

size_t get_zone_file_path(const char* control_type, const uint32_t* zones, uint32_t depth, powercap_zone_file type,
                          char* path, size_t size) {
  size_t written;
  int n;
  if ((written = get_base_path(control_type, zones, depth, path, size))) {
    n = zone_file_get_name(type, path + written, size - written);
    if (!snprintf_ret_to_size_t(n, size - written)) {
      return 0;
    }
    written += (size_t) n;
  }
  return written;
}

size_t get_constraint_file_path(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint,
                                powercap_constraint_file type, char* path, size_t size) {
  size_t written;
  int n;
  if ((written = get_base_path(control_type, zones, depth, path, size))) {
    n = constraint_file_get_name(type, constraint, path + written, size - written);
    if (!snprintf_ret_to_size_t(n, size - written)) {
      return 0;
    }
    written += (size_t) n;
  }
  return written;
}

int open_control_type_file(const char* control_type, powercap_control_type_file type, int flags) {
  char path[PATH_MAX];
  if (!get_control_type_file_path(control_type, type, path, sizeof(path))) {
    return -1;
  }
  return open(path, flags);
}

int open_zone_file(const char* control_type, const uint32_t* zones, uint32_t depth, powercap_zone_file type, int flags) {
  char path[PATH_MAX];
  if (!get_zone_file_path(control_type, zones, depth, type, path, sizeof(path))) {
    return -1;
  }
  return open(path, flags);
}

int open_constraint_file(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint,
                         powercap_constraint_file type, int flags) {
  char path[PATH_MAX];
  if (!get_constraint_file_path(control_type, zones, depth, constraint, type, path, sizeof(path))) {
    return -1;
  }
  return open(path, flags);
}

// like open(2), but returns 0 on ENOENT (No such file or directory)
static int powercap_open(const char* buf, int flags) {
  int fd = open(buf, flags);
  return (fd < 0 && errno == ENOENT) ? 0 : fd;
}

int powercap_control_type_file_open(char* buf, size_t bsize, const char* ct_name, powercap_control_type_file type,
                                    int flags) {
  return get_control_type_file_path(ct_name, type, buf, bsize) ? powercap_open(buf, flags) : -1;
}

int powercap_zone_file_open(char* buf, size_t bsize, const char* ct_name, const uint32_t* zones, uint32_t depth,
                            powercap_zone_file type, int flags) {
  return get_zone_file_path(ct_name, zones, depth, type, buf, bsize) ? powercap_open(buf, flags) : -1;
}

int powercap_constraint_file_open(char* buf, size_t bsize, const char* ct_name, const uint32_t* zones, uint32_t depth,
                                  uint32_t constraint, powercap_constraint_file type, int flags) {
  return get_constraint_file_path(ct_name, zones, depth, constraint, type, buf, bsize) ? powercap_open(buf, flags) : -1;
}

int powercap_control_type_open(powercap_control_type* pct, char* buf, size_t bsize, const char* ct_name, int ro) {
  return ((pct->enabled = powercap_control_type_file_open(buf, bsize, ct_name, POWERCAP_CONTROL_TYPE_FILE_ENABLED,
                                                          ro ? O_RDONLY : O_RDWR)) < 0)
         ? -1 : 0;
}

int powercap_zone_open(powercap_zone* pz, char* buf, size_t bsize, const char* ct_name, const uint32_t* zones,
                       uint32_t depth, int ro) {
  return ((pz->max_energy_range_uj =
            powercap_zone_file_open(buf, bsize, ct_name, zones, depth,
                                    POWERCAP_ZONE_FILE_MAX_ENERGY_RANGE_UJ, O_RDONLY)) < 0) ||
         // special case for energy_uj - it's allowed to be either RW or RO
         (
          ((pz->energy_uj =
              powercap_zone_file_open(buf, bsize, ct_name, zones, depth,
                                      POWERCAP_ZONE_FILE_ENERGY_UJ, ro ? O_RDONLY : O_RDWR)) < 0) &&
          (ro ||
            ((pz->energy_uj =
                powercap_zone_file_open(buf, bsize, ct_name, zones, depth,
                                        POWERCAP_ZONE_FILE_ENERGY_UJ, O_RDONLY)) < 0))
         ) ||
         ((pz->max_power_range_uw =
            powercap_zone_file_open(buf, bsize, ct_name, zones, depth,
                                    POWERCAP_ZONE_FILE_MAX_POWER_RANGE_UW, O_RDONLY)) < 0) ||
         ((pz->power_uw =
            powercap_zone_file_open(buf, bsize, ct_name, zones, depth,
                                    POWERCAP_ZONE_FILE_POWER_UW, O_RDONLY)) < 0) ||
         ((pz->enabled =
            powercap_zone_file_open(buf, bsize, ct_name, zones, depth,
                                    POWERCAP_ZONE_FILE_ENABLED, ro ? O_RDONLY : O_RDWR)) < 0) ||
         ((pz->name =
            powercap_zone_file_open(buf, bsize, ct_name, zones, depth,
                                    POWERCAP_ZONE_FILE_NAME, O_RDONLY)) < 0)
         ? -1 : 0;
}

int powercap_constraint_open(powercap_constraint* pc, char* buf, size_t bsize, const char* ct_name,
                             const uint32_t* zones, uint32_t depth, uint32_t constraint, int ro) {
  return ((pc->power_limit_uw =
            powercap_constraint_file_open(buf, bsize, ct_name, zones, depth, constraint,
                                          POWERCAP_CONSTRAINT_FILE_POWER_LIMIT_UW, ro ? O_RDONLY : O_RDWR)) < 0) ||
         ((pc->time_window_us =
            powercap_constraint_file_open(buf, bsize, ct_name, zones, depth, constraint,
                                          POWERCAP_CONSTRAINT_FILE_TIME_WINDOW_US, ro ? O_RDONLY : O_RDWR)) < 0) ||
         ((pc->max_power_uw =
            powercap_constraint_file_open(buf, bsize, ct_name, zones, depth, constraint,
                                          POWERCAP_CONSTRAINT_FILE_MAX_POWER_UW, O_RDONLY)) < 0) ||
         ((pc->min_power_uw =
            powercap_constraint_file_open(buf, bsize, ct_name, zones, depth, constraint,
                                          POWERCAP_CONSTRAINT_FILE_MIN_POWER_UW, O_RDONLY)) < 0) ||
         ((pc->max_time_window_us =
            powercap_constraint_file_open(buf, bsize, ct_name, zones, depth, constraint,
                                          POWERCAP_CONSTRAINT_FILE_MAX_TIME_WINDOW_US, O_RDONLY)) < 0) ||
         ((pc->min_time_window_us =
            powercap_constraint_file_open(buf, bsize, ct_name, zones, depth, constraint,
                                          POWERCAP_CONSTRAINT_FILE_MIN_TIME_WINDOW_US, O_RDONLY)) < 0) ||
         ((pc->name =
            powercap_constraint_file_open(buf, bsize, ct_name, zones, depth, constraint,
                                          POWERCAP_CONSTRAINT_FILE_NAME, O_RDONLY)) < 0)
         ? -1 : 0;
}

static int powercap_close(int fd) {
  return (fd > 0 && close(fd)) ? -1 : 0;
}

int powercap_control_type_close(powercap_control_type* pct) {
  return powercap_close(pct->enabled);
}

int powercap_zone_close(powercap_zone* pz) {
  int rc = 0;
  rc |= powercap_close(pz->max_energy_range_uj);
  rc |= powercap_close(pz->energy_uj);
  rc |= powercap_close(pz->max_power_range_uw);
  rc |= powercap_close(pz->power_uw);
  rc |= powercap_close(pz->enabled);
  rc |= powercap_close(pz->name);
  return rc;
}

int powercap_constraint_close(powercap_constraint* pc) {
  int rc = 0;
  rc |= powercap_close(pc->power_limit_uw);
  rc |= powercap_close(pc->time_window_us);
  rc |= powercap_close(pc->max_power_uw);
  rc |= powercap_close(pc->min_power_uw);
  rc |= powercap_close(pc->max_time_window_us);
  rc |= powercap_close(pc->min_time_window_us);
  rc |= powercap_close(pc->name);
  return rc;
}
