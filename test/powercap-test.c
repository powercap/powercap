/**
 * Basic unit tests.
 * Can't actually test the implementation without a system that supports all file types.
 */
// force assertions
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "powercap.h"

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

int main(void) {
  test_powercap_zone_file_get_name();
  test_powercap_constraint_file_get_name();
  return EXIT_SUCCESS;
}
