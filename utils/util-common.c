/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Common utilities.
 *
 * @author Connor Imes
 * @date 2017-08-24
 */
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util-common.h"

#define INDENT "  "

static int fail_invalid(int* cont) {
  if (cont) {
    *cont = 0;
  }
  return -EINVAL;
}

static int parse_status(const char* optarg, const char* end, int* cont) {
  /* Not an empty string, out of range, or having extra characters */
  if (optarg == end || errno == ERANGE || end != optarg + strlen(optarg)) {
    return fail_invalid(cont);
  }
  return 0;
}

static int parse_u32(const char* optarg, uint32_t* val, int* cont) {
  char* end;
  errno = 0;
  *val = strtoul(optarg, &end, 0);
  return parse_status(optarg, end, cont);
}

static int parse_u64(const char* optarg, uint64_t* val, int* cont) {
  char* end;
  errno = 0;
  *val = strtoull(optarg, &end, 0);
  return parse_status(optarg, end, cont);
}

int set_u32_param(u32_param* p, const char* optarg, int* cont) {
  if (p->set || !optarg) {
    return fail_invalid(cont);
  }
  p->set = 1;
  return parse_u32(optarg, &p->val, cont);
}

int set_u64_param(u64_param* p, const char* optarg, int* cont) {
  if (p->set || !optarg) {
    return fail_invalid(cont);
  }
  p->set = 1;
  return parse_u64(optarg, &p->val, cont);
}

int parse_zones(const char* optarg, uint32_t* zones, uint32_t max_depth, uint32_t* depth, int* cont) {
  char* z;
  char* ptr;
  char* copy;
  int ret = -EINVAL;
  /* it's an error if depth is already set (zones were already parsed) */
  if (*depth || !optarg) {
    return fail_invalid(cont);
  }
  if ((copy = strdup(optarg))) {
    z = strtok_r(copy, ":", &ptr);
    for (*depth = 0; z; (*depth)++) {
      if (*depth == max_depth) {
        ret = -ENOBUFS;
        break;
      }
      if ((ret = parse_u32(z, &zones[*depth], cont))) {
        break;
      }
      z = strtok_r(NULL, ":", &ptr);
    }
    free(copy);
  } else {
    ret = -ENOMEM;
  }
  if (ret && cont) {
    *cont = 0;
  }
  return ret;
}

void indent(uint32_t n) {
  while (n-- > 0) {
    printf(INDENT);
  }
}

static void maybe_verbose(int verbose, uint32_t in, const char* base) {
  if (verbose) {
    indent(in);
    printf("%s: %s\n", base, strerror(errno));
  }
}

void str_or_verbose(int verbose, uint32_t in, const char* base, const char* val, int retval) {
  if (retval) {
    maybe_verbose(verbose, in, base);
  } else {
    indent(in);
    printf("%s: %s\n", base, val);
  }
}

void u64_or_verbose(int verbose, uint32_t in, const char* base, uint64_t val, int retval) {
  if (retval) {
    maybe_verbose(verbose, in, base);
  } else {
    indent(in);
    printf("%s: %"PRIu64"\n", base, val);
  }
}

int is_valid_powercap_control_type(const char* control_type) {
  return control_type && strlen(control_type) && strcspn(control_type, "./") == strlen(control_type);
}

int get_recurse(char* optarg) {
  size_t len;
  /* Default is to recurse */
  int recurse = 1;
  if (optarg && (len = strlen(optarg)) && optarg[len - 1] == ':') {
    recurse = 0;
    /* Remove trailing colon */
    optarg[len - 1] = '\0';
  }
  return recurse;
}
