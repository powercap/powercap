/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Common utilities.
 *
 * @author Connor Imes
 * @date 2017-08-24
 */

#ifndef _UTIL_COMMON_H
#define _UTIL_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#ifndef MAX_ZONE_DEPTH
/* A large number */
#define MAX_ZONE_DEPTH 64
#endif // MAX_ZONE_DEPTH
#ifndef MAX_NAME_SIZE
#define MAX_NAME_SIZE 1024
#endif // MAX_NAME_SIZE

typedef struct u32_param {
  uint32_t val;
  int set;
} u32_param;

typedef struct u64_param {
  uint64_t val;
  int set;
} u64_param;

int set_u32_param(u32_param* p, const char* optarg, int* cont);

int set_u64_param(u64_param* p, const char* optarg, int* cont);

int parse_zones(const char* optarg, uint32_t* zones, uint32_t max_depth, uint32_t* depth, int* cont);

void indent(uint32_t n);

void str_or_verbose(int verbose, uint32_t in, const char* base, const char* val, int retval);

void u64_or_verbose(int verbose, uint32_t in, const char* base, uint64_t val, int retval);

int is_valid_powercap_control_type(const char* control_type);

int get_recurse(char* optarg);

#ifdef __cplusplus
}
#endif

#endif
