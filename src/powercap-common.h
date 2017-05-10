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

#include <stdio.h>

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

#define LOG(severity, args...) \
  do { if ((severity) >= POWERCAP_LOG_LEVEL) { \
      fprintf(TO_FILE((severity)), "%s [powercap] ", TO_LOG_PREFIX((severity))); \
      fprintf(TO_FILE((severity)), args); \
    } } while (0)

#ifdef __cplusplus
}
#endif

#endif
