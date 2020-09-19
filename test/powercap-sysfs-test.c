/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Tests bad parameters.
 * No way to test good ones without a functioning powercap implementation, which isn't guaranteed to exist.
 */
/* force assertions */
#undef NDEBUG
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "powercap-sysfs.h"

/* PATH_MAX should be defined in limits.h */
#ifndef PATH_MAX
  #pragma message("Warning: PATH_MAX was not defined")
  #define PATH_MAX 4096
#endif

static void test_bad_control_type_exists(void) {
  char buf_too_big[PATH_MAX + 1];
  size_t i;
  /* bad params */
  errno = 0;
  assert(powercap_sysfs_control_type_exists(NULL) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_control_type_exists("") == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_control_type_exists(".") == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_control_type_exists("/") == -EINVAL);
  assert(errno == EINVAL);
  for (i = 0; i < sizeof(buf_too_big); i++) {
    buf_too_big[i] = 'a';
  }
  buf_too_big[i - 1] = '\0';
  errno = 0;
  assert(powercap_sysfs_control_type_exists(buf_too_big) == -ENOBUFS);
  assert(errno == ENOBUFS);
  /* good param, but doesn't exist */
  errno = 0;
  assert(powercap_sysfs_control_type_exists("foo") == -ENOSYS);
  assert(errno == ENOSYS);
}

static void test_bad_zone_exists(void) {
  uint32_t zones[2];
  zones[0] = 0;
  zones[1] = 0;
  /* good parameters, bad control type */
  errno = 0;
  assert(powercap_sysfs_zone_exists("foo", zones, 2) == -ENOSYS);
  assert(errno == ENOSYS);
  /* bad parameters */
  errno = 0;
  assert(powercap_sysfs_zone_exists(NULL, zones, 2) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_zone_exists("foo", NULL, 2) == -EINVAL);
  assert(errno == EINVAL);
}

static void test_bad_constraint_exists(void) {
  uint32_t zones[2];
  zones[0] = 0;
  zones[1] = 0;
  /* good parameters, bad control type */
  errno = 0;
  assert(powercap_sysfs_constraint_exists("foo", zones, 2, 0) == -ENOSYS);
  assert(errno == ENOSYS);
  /* bad parameters */
  errno = 0;
  assert(powercap_sysfs_constraint_exists(NULL, zones, 2, 0) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_constraint_exists("foo", NULL, 2, 0) == -EINVAL);
  assert(errno == EINVAL);
}

static void test_get_set_control_type_all_bad(void) {
  uint32_t val32;
  /* Good parameters, bad control type */
  errno = 0;
  assert(powercap_sysfs_control_type_get_enabled("foo", &val32) == -ENOENT);
  assert(errno == ENOENT);
  errno = 0;
  assert(powercap_sysfs_control_type_set_enabled("foo", 1) == -ENOENT);
  assert(errno == ENOENT);
  /* Bad parameters */
  /* get u32 */
  errno = 0;
  assert(powercap_sysfs_control_type_get_enabled(NULL, &val32) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_control_type_get_enabled("foo", NULL) == -EINVAL);
  assert(errno == EINVAL);
  /* set u32 */
  errno = 0;
  assert(powercap_sysfs_control_type_set_enabled(NULL, 1) == -EINVAL);
  assert(errno == EINVAL);
}

static void test_get_set_zone_all_bad(void) {
  uint64_t val64;
  uint32_t val32;
  char name[32];
  uint32_t zones[2];
  zones[0] = 0;
  zones[1] = 0;
  /* Good parameters, bad control type */
  errno = 0;
  assert(powercap_sysfs_zone_get_max_energy_range_uj("foo", zones, 2, &val64) == -ENOENT);
  assert(errno == ENOENT);
  errno = 0;
  assert(powercap_sysfs_zone_get_energy_uj("foo", zones, 2, &val64) == -ENOENT);
  assert(errno == ENOENT);
  errno = 0;
  assert(powercap_sysfs_zone_reset_energy_uj("foo", zones, 2) == -ENOENT);
  assert(errno == ENOENT);
  errno = 0;
  assert(powercap_sysfs_zone_get_max_power_range_uw("foo", zones, 2, &val64) == -ENOENT);
  assert(errno == ENOENT);
  errno = 0;
  assert(powercap_sysfs_zone_get_power_uw("foo", zones, 2, &val64) == -ENOENT);
  assert(errno == ENOENT);
  errno = 0;
  assert(powercap_sysfs_zone_get_enabled("foo", zones, 2, &val32) == -ENOENT);
  assert(errno == ENOENT);
  errno = 0;
  assert(powercap_sysfs_zone_set_enabled("foo", zones, 2, 1) == -ENOENT);
  assert(errno == ENOENT);
  errno = 0;
  assert(powercap_sysfs_zone_get_name("foo", zones, 2, name, sizeof(name)) == -ENOENT);
  assert(errno == ENOENT);
  /* Bad parameters */
  /* Just test one of each function type */
  /* get u64 */
  errno = 0;
  assert(powercap_sysfs_zone_get_max_energy_range_uj(NULL, zones, 2, &val64) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_zone_get_max_energy_range_uj("foo", NULL, 2, &val64) == -EINVAL);
  assert(errno == EINVAL);
#if 0
  /* TODO: Can't test NULL value parameters since we can't get past opening the file */
  errno = 0;
  assert(powercap_sysfs_zone_get_max_energy_range_uj("foo", zones, 2, NULL) == -EINVAL);
  assert(errno == EINVAL);
#endif
  /* set u64 */
  errno = 0;
  assert(powercap_sysfs_zone_reset_energy_uj(NULL, zones, 2) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_zone_reset_energy_uj("foo", NULL, 2) == -EINVAL);
  assert(errno == EINVAL);
  /* get u32 */
  errno = 0;
  assert(powercap_sysfs_zone_get_enabled(NULL, zones, 2, &val32) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_zone_get_enabled("foo", NULL, 2, &val32) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_zone_get_enabled("foo", zones, 2, NULL) == -EINVAL);
  assert(errno == EINVAL);
  /* set u32 */
  errno = 0;
  assert(powercap_sysfs_zone_set_enabled(NULL, zones, 2, 1) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_zone_set_enabled("foo", NULL, 2, 1) == -EINVAL);
  assert(errno == EINVAL);
  /* get string */
  errno = 0;
  assert(powercap_sysfs_zone_get_name(NULL, zones, 2, name, sizeof(name)) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_zone_get_name("foo", NULL, 2, name, sizeof(name)) == -EINVAL);
  assert(errno == EINVAL);
#if 0
  /* TODO: Can't test bad name or size parameters since we can't get past opening the file */
  errno = 0;
  assert(powercap_sysfs_zone_get_name("foo", zones, 2, NULL, sizeof(name)) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_zone_get_name("foo", zones, 2, name, 0) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_zone_get_name("foo", zones, 2, name, 1) == -EINVAL);
  assert(errno == EINVAL);
#endif
}

static void test_get_set_constraint_all_bad(void) {
  uint64_t val64;
  char name[32];
  uint32_t zones[2];
  zones[0] = 0;
  zones[1] = 0;
  /* Good parameters, bad control type */
  errno = 0;
  assert(powercap_sysfs_constraint_set_power_limit_uw("foo", zones, 2, 0, 0) == -ENOENT);
  assert(errno == ENOENT);
  errno = 0;
  assert(powercap_sysfs_constraint_get_power_limit_uw("foo", zones, 2, 0, &val64) == -ENOENT);
  assert(errno == ENOENT);
  errno = 0;
  assert(powercap_sysfs_constraint_set_time_window_us("foo", zones, 2, 0, 0) == -ENOENT);
  assert(errno == ENOENT);
  errno = 0;
  assert(powercap_sysfs_constraint_get_time_window_us("foo", zones, 2, 0, &val64) == -ENOENT);
  assert(errno == ENOENT);
  errno = 0;
  assert(powercap_sysfs_constraint_get_max_power_uw("foo", zones, 2, 0, &val64) == -ENOENT);
  assert(errno == ENOENT);
  errno = 0;
  assert(powercap_sysfs_constraint_get_min_power_uw("foo", zones, 2, 0, &val64) == -ENOENT);
  assert(errno == ENOENT);
  errno = 0;
  assert(powercap_sysfs_constraint_get_max_time_window_us("foo", zones, 2, 0, &val64) == -ENOENT);
  assert(errno == ENOENT);
  errno = 0;
  assert(powercap_sysfs_constraint_get_min_time_window_us("foo", zones, 2, 0, &val64) == -ENOENT);
  assert(errno == ENOENT);
  errno = 0;
  assert(powercap_sysfs_constraint_get_name("foo", zones, 2, 0, name, sizeof(name)) == -ENOENT);
  assert(errno == ENOENT);
  /* Bad parameters */
  /* Just test one of each function type */
  /* set u64 */
  errno = 0;
  assert(powercap_sysfs_constraint_set_power_limit_uw(NULL, zones, 2, 0, 0) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_constraint_set_power_limit_uw("foo", NULL, 2, 0, 0) == -EINVAL);
  assert(errno == EINVAL);
  /* get u64 */
  errno = 0;
  assert(powercap_sysfs_constraint_get_power_limit_uw(NULL, zones, 2, 0, &val64) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_constraint_get_power_limit_uw("foo", NULL, 2, 0, &val64) == -EINVAL);
  assert(errno == EINVAL);
#if 0
  /* TODO: Can't test NULL value parameters since we can't get past opening the file */
  errno = 0;
  assert(powercap_sysfs_constraint_get_power_limit_uw("foo", zones, 2, 0, NULL) == -EINVAL);
  assert(errno == EINVAL);
#endif
  /* get string */
  errno = 0;
  assert(powercap_sysfs_constraint_get_name(NULL, zones, 2, 0, name, sizeof(name)) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_constraint_get_name("foo", NULL, 2, 0, name, sizeof(name)) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
#if 0
  /* TODO: Can't test bad name or size parameters since we can't get past opening the file */
  assert(powercap_sysfs_constraint_get_name("foo", zones, 2, 0, NULL, sizeof(name)) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_constraint_get_name("foo", zones, 2, 0, name, 0) == -EINVAL);
  assert(errno == EINVAL);
  errno = 0;
  assert(powercap_sysfs_constraint_get_name("foo", zones, 2, 0, name, 1) == -EINVAL);
  assert(errno == EINVAL);
#endif
}

int main(void) {
  test_bad_control_type_exists();
  test_bad_zone_exists();
  test_bad_constraint_exists();
  test_get_set_control_type_all_bad();
  test_get_set_zone_all_bad();
  test_get_set_constraint_all_bad();
  return EXIT_SUCCESS;
}
