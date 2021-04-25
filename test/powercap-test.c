/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Basic unit tests.
 * Can't actually test the implementation without a system that supports all file types.
 */
// force assertions
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "powercap.h"

#ifdef USE_VIRTUAL_DEVICES
  #define POWERCAP_PATH "/sys/devices/virtual/powercap"
#else
  #define POWERCAP_PATH "/sys/class/powercap"
#endif

static void test_powercap_control_type_file_get_name(void) {
  char buf[24];
  assert(powercap_control_type_file_get_name(POWERCAP_CONTROL_TYPE_FILE_ENABLED, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, "enabled", sizeof(buf)) == 0);
}

static void test_powercap_zone_file_get_name(void) {
  char buf[24];
  assert(powercap_zone_file_get_name(POWERCAP_ZONE_FILE_MAX_ENERGY_RANGE_UJ, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, "max_energy_range_uj", sizeof(buf)) == 0);
  assert(powercap_zone_file_get_name(POWERCAP_ZONE_FILE_ENERGY_UJ, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, "energy_uj", sizeof(buf)) == 0);
  assert(powercap_zone_file_get_name(POWERCAP_ZONE_FILE_MAX_POWER_RANGE_UW, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, "max_power_range_uw", sizeof(buf)) == 0);
  assert(powercap_zone_file_get_name(POWERCAP_ZONE_FILE_POWER_UW, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, "power_uw", sizeof(buf)) == 0);
  assert(powercap_zone_file_get_name(POWERCAP_ZONE_FILE_ENABLED, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, "enabled", sizeof(buf)) == 0);
  assert(powercap_zone_file_get_name(POWERCAP_ZONE_FILE_NAME, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, "name", sizeof(buf)) == 0);
}

static void test_powercap_constraint_file_get_name(void) {
  char buf[32];
  assert(powercap_constraint_file_get_name(POWERCAP_CONSTRAINT_FILE_POWER_LIMIT_UW, 0, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, "constraint_0_power_limit_uw", sizeof(buf)) == 0);
  assert(powercap_constraint_file_get_name(POWERCAP_CONSTRAINT_FILE_TIME_WINDOW_US, 0, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, "constraint_0_time_window_us", sizeof(buf)) == 0);
  assert(powercap_constraint_file_get_name(POWERCAP_CONSTRAINT_FILE_MAX_POWER_UW, 0, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, "constraint_0_max_power_uw", sizeof(buf)) == 0);
  assert(powercap_constraint_file_get_name(POWERCAP_CONSTRAINT_FILE_MIN_POWER_UW, 0, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, "constraint_0_min_power_uw", sizeof(buf)) == 0);
  assert(powercap_constraint_file_get_name(POWERCAP_CONSTRAINT_FILE_MAX_TIME_WINDOW_US, 0, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, "constraint_0_max_time_window_us", sizeof(buf)) == 0);
  assert(powercap_constraint_file_get_name(POWERCAP_CONSTRAINT_FILE_MIN_TIME_WINDOW_US, 0, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, "constraint_0_min_time_window_us", sizeof(buf)) == 0);
  assert(powercap_constraint_file_get_name(POWERCAP_CONSTRAINT_FILE_NAME, 0, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, "constraint_0_name", sizeof(buf)) == 0);
}

#if 0
static void test_powercap_get_path(void) {
  char buf[4096];
  uint32_t zones[2];
  assert(powercap_get_path("foo", NULL, 0, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/", sizeof(buf)) == 0);
  zones[0] = 1;
  assert(powercap_get_path("foo", zones, 1, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/foo:1/", sizeof(buf)) == 0);
  zones[1] = 2;
  assert(powercap_get_path("foo", zones, 2, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/foo:1/foo:1:2/", sizeof(buf)) == 0);
  // should print zone index values in lower case hexadecimal, not decimal
  zones[1] = 10;
  assert(powercap_get_path("foo", zones, 2, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/foo:1/foo:1:a/", sizeof(buf)) == 0);
}

static void test_powercap_control_type_file_get_path(void) {
  char buf[4096];
  assert(powercap_control_type_file_get_path(POWERCAP_CONTROL_TYPE_FILE_ENABLED, "foo", buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/enabled", sizeof(buf)) == 0);
}

static void test_powercap_zone_file_get_path(void) {
  char buf[4096];
  uint32_t zones = 5;
  assert(powercap_zone_file_get_path(POWERCAP_ZONE_FILE_MAX_ENERGY_RANGE_UJ, "foo", &zones, 1, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/foo:5/max_energy_range_uj", sizeof(buf)) == 0);
  assert(powercap_zone_file_get_path(POWERCAP_ZONE_FILE_ENERGY_UJ, "foo", &zones, 1, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/foo:5/energy_uj", sizeof(buf)) == 0);
  assert(powercap_zone_file_get_path(POWERCAP_ZONE_FILE_MAX_POWER_RANGE_UW, "foo", &zones, 1, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/foo:5/max_power_range_uw", sizeof(buf)) == 0);
  assert(powercap_zone_file_get_path(POWERCAP_ZONE_FILE_POWER_UW, "foo", &zones, 1, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/foo:5/power_uw", sizeof(buf)) == 0);
  assert(powercap_zone_file_get_path(POWERCAP_ZONE_FILE_ENABLED, "foo", &zones, 1, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/foo:5/enabled", sizeof(buf)) == 0);
  assert(powercap_zone_file_get_path(POWERCAP_ZONE_FILE_NAME, "foo", &zones, 1, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/foo:5/name", sizeof(buf)) == 0);
}

static void test_powercap_constraint_file_get_path(void) {
  char buf[4096];
  uint32_t zones = 3;
  assert(powercap_constraint_file_get_path(POWERCAP_CONSTRAINT_FILE_POWER_LIMIT_UW, "foo", &zones, 1, 7, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/foo:3/constraint_7_power_limit_uw", sizeof(buf)) == 0);
  assert(powercap_constraint_file_get_path(POWERCAP_CONSTRAINT_FILE_TIME_WINDOW_US, "foo", &zones, 1, 7, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/foo:3/constraint_7_time_window_us", sizeof(buf)) == 0);
  assert(powercap_constraint_file_get_path(POWERCAP_CONSTRAINT_FILE_MAX_POWER_UW, "foo", &zones, 1, 7, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/foo:3/constraint_7_max_power_uw", sizeof(buf)) == 0);
  assert(powercap_constraint_file_get_path(POWERCAP_CONSTRAINT_FILE_MIN_POWER_UW, "foo", &zones, 1, 7, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/foo:3/constraint_7_min_power_uw", sizeof(buf)) == 0);
  assert(powercap_constraint_file_get_path(POWERCAP_CONSTRAINT_FILE_MAX_TIME_WINDOW_US, "foo", &zones, 1, 7, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/foo:3/constraint_7_max_time_window_us", sizeof(buf)) == 0);
  assert(powercap_constraint_file_get_path(POWERCAP_CONSTRAINT_FILE_MIN_TIME_WINDOW_US, "foo", &zones, 1, 7, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/foo:3/constraint_7_min_time_window_us", sizeof(buf)) == 0);
  assert(powercap_constraint_file_get_path(POWERCAP_CONSTRAINT_FILE_NAME, "foo", &zones, 1, 7, buf, sizeof(buf)) > 0);
  assert(strncmp(buf, POWERCAP_PATH"/foo/foo:3/constraint_7_name", sizeof(buf)) == 0);
}
#endif

int main(void) {
  test_powercap_control_type_file_get_name();
  test_powercap_zone_file_get_name();
  test_powercap_constraint_file_get_name();
#if 0
  test_powercap_get_path();
  test_powercap_control_type_file_get_path();
  test_powercap_zone_file_get_path();
  test_powercap_constraint_file_get_path();
#endif
  return EXIT_SUCCESS;
}
