/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Basic unit tests for powercap-common.
 */
// force assertions
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "powercap.h"
#include "../src/powercap-common.h"

#ifdef USE_VIRTUAL_DEVICES
  #define POWERCAP_PATH "/sys/devices/virtual/powercap"
#else
  #define POWERCAP_PATH "/sys/class/powercap"
#endif
#define POWERCAP_PATH_LEN (sizeof(POWERCAP_PATH) - 1)

#define CONTROL_TYPE "foo"
#define CONTROL_TYPE_LEN (sizeof(CONTROL_TYPE) - 1)

#define ROOT_LEN (POWERCAP_PATH_LEN + 1 + CONTROL_TYPE_LEN + 1)
#define BASE_D1_LEN (ROOT_LEN + CONTROL_TYPE_LEN + 3)
#define BASE_D2_LEN (BASE_D1_LEN + CONTROL_TYPE_LEN + 5)

static void test_snprintf_base_path(void) {
  char path[PATH_MAX] = { 0 };
  uint32_t zones[PATH_MAX] = { 0 }; // can produce paths that exceed PATH_MAX length
  int rc;
  // root path
  assert(snprintf_base_path(path, sizeof(path), CONTROL_TYPE, NULL, 0) == ROOT_LEN);
  assert(strncmp(path, POWERCAP_PATH"/"CONTROL_TYPE"/", sizeof(path)) == 0);
  // depth 1
  zones[0] = 0;
  assert(snprintf_base_path(path, sizeof(path), CONTROL_TYPE, zones, 1) == BASE_D1_LEN);
  assert(strncmp(path, POWERCAP_PATH"/"CONTROL_TYPE"/"CONTROL_TYPE":0/", sizeof(path)) == 0);
  // depth 2
  zones[0] = 0;
  zones[1] = 1;
  assert(snprintf_base_path(path, sizeof(path), CONTROL_TYPE, zones, 2) == BASE_D2_LEN);
  assert(strncmp(path, POWERCAP_PATH"/"CONTROL_TYPE"/"CONTROL_TYPE":0/"CONTROL_TYPE":0:1/", sizeof(path)) == 0);
  // too long to fit in buffer
  rc = snprintf_base_path(path, sizeof(path), CONTROL_TYPE, zones, PATH_MAX);
  assert(rc >= 0 && (size_t) rc > sizeof(path)); // can't safely cast to size_t without checking >= 0 first
}

static void test_snprintf_control_type_file(void) {
  char path[PATH_MAX] = { 0 };
  assert(snprintf_control_type_file(path, sizeof(path), POWERCAP_CONTROL_TYPE_FILE_ENABLED) == 7);
  assert(strncmp(path, "enabled", sizeof(path)) == 0);
}

static void test_snprintf_zone_file(void) {
  char path[PATH_MAX] = { 0 };
  assert(snprintf_zone_file(path, sizeof(path), POWERCAP_ZONE_FILE_MAX_ENERGY_RANGE_UJ) == 19);
  assert(strncmp(path, "max_energy_range_uj", sizeof(path)) == 0);
  assert(snprintf_zone_file(path, sizeof(path), POWERCAP_ZONE_FILE_ENERGY_UJ) == 9);
  assert(strncmp(path, "energy_uj", sizeof(path)) == 0);
  assert(snprintf_zone_file(path, sizeof(path), POWERCAP_ZONE_FILE_MAX_POWER_RANGE_UW) == 18);
  assert(strncmp(path, "max_power_range_uw", sizeof(path)) == 0);
  assert(snprintf_zone_file(path, sizeof(path), POWERCAP_ZONE_FILE_POWER_UW) == 8);
  assert(strncmp(path, "power_uw", sizeof(path)) == 0);
  assert(snprintf_zone_file(path, sizeof(path), POWERCAP_ZONE_FILE_ENABLED) == 7);
  assert(strncmp(path, "enabled", sizeof(path)) == 0);
  assert(snprintf_zone_file(path, sizeof(path), POWERCAP_ZONE_FILE_NAME) == 4);
  assert(strncmp(path, "name", sizeof(path)) == 0);
}

static void test_snprintf_constraint_file(void) {
  char path[PATH_MAX] = { 0 };
  assert(snprintf_constraint_file(path, sizeof(path), POWERCAP_CONSTRAINT_FILE_POWER_LIMIT_UW, 0) == 27);
  assert(strncmp(path, "constraint_0_power_limit_uw", sizeof(path)) == 0);
  assert(snprintf_constraint_file(path, sizeof(path), POWERCAP_CONSTRAINT_FILE_TIME_WINDOW_US, 0) == 27);
  assert(strncmp(path, "constraint_0_time_window_us", sizeof(path)) == 0);
  assert(snprintf_constraint_file(path, sizeof(path), POWERCAP_CONSTRAINT_FILE_MAX_POWER_UW, 0) == 25);
  assert(strncmp(path, "constraint_0_max_power_uw", sizeof(path)) == 0);
  assert(snprintf_constraint_file(path, sizeof(path), POWERCAP_CONSTRAINT_FILE_MIN_POWER_UW, 0) == 25);
  assert(strncmp(path, "constraint_0_min_power_uw", sizeof(path)) == 0);
  assert(snprintf_constraint_file(path, sizeof(path), POWERCAP_CONSTRAINT_FILE_MAX_TIME_WINDOW_US, 0) == 31);
  assert(strncmp(path, "constraint_0_max_time_window_us", sizeof(path)) == 0);
  assert(snprintf_constraint_file(path, sizeof(path), POWERCAP_CONSTRAINT_FILE_MIN_TIME_WINDOW_US, 0) == 31);
  assert(strncmp(path, "constraint_0_min_time_window_us", sizeof(path)) == 0);
  assert(snprintf_constraint_file(path, sizeof(path), POWERCAP_CONSTRAINT_FILE_NAME, 0) == 17);
  assert(strncmp(path, "constraint_0_name", sizeof(path)) == 0);
}

static void test_snprintf_control_type_file_path(void) {
  char path[PATH_MAX] = { 0 };
  assert(snprintf_control_type_file_path(path, sizeof(path), CONTROL_TYPE, POWERCAP_CONTROL_TYPE_FILE_ENABLED) == ROOT_LEN + 7);
  assert(strncmp(path, POWERCAP_PATH"/"CONTROL_TYPE"/enabled", sizeof(path)) == 0);
}

static void test_snprintf_zone_file_path(void) {
  char path[PATH_MAX] = { 0 };
  uint32_t zones[1] = { 0 };
  // we really just need to test one type
  assert(snprintf_zone_file_path(path, sizeof(path), CONTROL_TYPE, zones, 1, POWERCAP_ZONE_FILE_MAX_ENERGY_RANGE_UJ) == BASE_D1_LEN + 19);
  assert(strncmp(path, POWERCAP_PATH"/"CONTROL_TYPE"/"CONTROL_TYPE":0/max_energy_range_uj", sizeof(path)) == 0);
}

static void test_snprintf_constraint_file_path(void) {
  char path[PATH_MAX] = { 0 };
  uint32_t zones[1] = { 0 };
  // we really just need to test one type
  assert(snprintf_constraint_file_path(path, sizeof(path), CONTROL_TYPE, zones, 1, POWERCAP_CONSTRAINT_FILE_POWER_LIMIT_UW, 0) == BASE_D1_LEN + 27);
  assert(strncmp(path, POWERCAP_PATH"/"CONTROL_TYPE"/"CONTROL_TYPE":0/constraint_0_power_limit_uw", sizeof(path)) == 0);
}

int main(void) {
  // We can really only test the snprintf functions in a unit test, not actual file I/O
  test_snprintf_base_path();
  test_snprintf_control_type_file();
  test_snprintf_zone_file();
  test_snprintf_constraint_file();
  test_snprintf_control_type_file_path();
  test_snprintf_zone_file_path();
  test_snprintf_constraint_file_path();
  return EXIT_SUCCESS;
}
