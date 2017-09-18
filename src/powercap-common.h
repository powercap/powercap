/**
 * Common utilities, like logging.
 *
 * @author Connor Imes
 * @date 2017-05-09
 */
#ifndef _POWERCAP_COMMON_H_
#define _POWERCAP_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>
#include <stdio.h>
/* Main powercap header only used for enums */
#include "powercap.h"

typedef enum powercap_loglevel {
  DEBUG = 0,
  INFO,
  WARN,
  ERROR,
  OFF,
} powercap_loglevel;

#ifndef POWERCAP_LOG_LEVEL
  #define POWERCAP_LOG_LEVEL WARN
#endif

#define TO_FILE(severity) (severity) >= WARN ? stderr : stdout

#define TO_LOG_PREFIX(severity) \
  (severity) == DEBUG ? "[DEBUG]" : \
  (severity) == INFO  ? "[INFO] " : \
  (severity) == WARN  ? "[WARN] " : \
                        "[ERROR]"

#define LOG(severity, ...) \
  do { if ((severity) >= POWERCAP_LOG_LEVEL) { \
      fprintf(TO_FILE((severity)), "%s [powercap] ", TO_LOG_PREFIX((severity))); \
      fprintf(TO_FILE((severity)), __VA_ARGS__); \
    } } while (0)

/* PATH_MAX should be defined in limits.h */
#ifndef PATH_MAX
  #pragma message("Warning: PATH_MAX was not defined")
  #define PATH_MAX 4096
#endif

/* buf must not be NULL and size >= 1 */
ssize_t read_string_safe(int fd, char* buf, size_t size);

/* Return number of bytes read (including terminating NULL char) on success, negative error code on failure */
ssize_t read_string(int fd, char* buf, size_t size);

/* Return 0 on success, negative error code on failure */
int read_u64(int fd, uint64_t* val);

/* Return 0 on success, negative error code on failure */
int write_u64(int fd, uint64_t val);

/* Return is like snprintf, or negative error code if parameters are bad */
int zone_file_get_name(powercap_zone_file type, char* buf, size_t size);

/* Return is like snprintf, or negative error code if parameters are bad */
int constraint_file_get_name(powercap_constraint_file type, uint32_t constraint, char* buf, size_t size);

/*
 * Returns 0 on failure like insufficient buffer size or if control_type is NULL.
 * zones can be NULL only if depth is 0; path must not be NULL.
 */
size_t get_base_path(const char* control_type, const uint32_t* zones, uint32_t depth, char* path, size_t size);

/* Returns 0 on failure like insufficient buffer size */
size_t get_zone_file_path(const char* control_type, const uint32_t* zones, uint32_t depth, powercap_zone_file type,
                          char* path, size_t size);

/* Returns 0 on failure like insufficient buffer size */
size_t get_constraint_file_path(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint,
                                powercap_constraint_file type, char* path, size_t size);

/* Return fd on success, negative error code if path is too large, -1 on open failure */
int open_zone_file(const char* control_type, const uint32_t* zones, uint32_t depth, powercap_zone_file type, int flags);

/* Return fd on success, negative error code if path is too large, -1 on open failure */
int open_constraint_file(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint,
                         powercap_constraint_file type, int flags);

#ifdef __cplusplus
}
#endif

#endif
