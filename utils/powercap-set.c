/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Set powercap values.
 *
 * @author Connor Imes
 * @date 2017-08-24
 */
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "powercap-sysfs.h"
#include "util-common.h"

static const char short_options[] = "-hp:z:c:E:je:l:s:";

static const struct option long_options[] = {
  {"help",                no_argument,        NULL, 'h'},
  {"control-type",        required_argument,  NULL, 'p'},
  {"zone",                required_argument,  NULL, 'z'},
  {"constraint",          required_argument,  NULL, 'c'},
  {"enabled",             required_argument,  NULL, 'E'},
  {"z-energy",            no_argument      ,  NULL, 'j'},
  {"z-enabled",           required_argument,  NULL, 'e'},
  {"c-power-limit",       required_argument,  NULL, 'l'},
  {"c-time-window",       required_argument,  NULL, 's'},
  {0, 0, 0, 0}
};

static void print_usage(void) {
  printf("Usage: powercap-set NAME [OPTION]...\n\n");
  printf("Sets configurations for a powercap control type.\n");
  printf("The control type NAME must not be empty or contain a '.' or '/'.\n\n");
  printf("Options:\n");
  printf("  -h, --help                   Print this message and exit\n");
  printf("  -p, --control-type=NAME      Deprecated, provide NAME as the first\n");
  printf("                               positional argument instead\n");
  printf("  -z, --zone=ZONE(S)           The zone/subzone numbers in the control type's\n");
  printf("                               powercap tree\n");
  printf("                               Separate zones/subzones with a colon\n");
  printf("                               E.g., for zone 0, subzone 2: \"-z 0:2\"\n");
  printf("  -c, --constraint=CONSTRAINT  The constraint number (none by default)\n");
  printf("  -E, --enabled=1|0            Enable/disable the control type\n");
  printf("The following zone-level arguments may be used together and require -z/--zone:\n");
  printf("  -j, --z-energy               Reset zone energy counter\n");
  printf("  -e, --z-enabled=1|0          Enable/disable a zone\n");
  printf("The following constraint-level arguments may be used together and require -z/--zone and -c/--constraint:\n");
  printf("  -l, --c-power-limit=UW       Set constraint power limit\n");
  printf("  -s, --c-time-window=US       Set constraint time window\n");
  printf("\nPower units: microwatts (uW)\n");
  printf("Time units: microseconds (us)\n");
}

static void print_common_help(void) {
  printf("Considerations for common errors:\n");
  printf("- Ensure that the control type exists, which may require loading a kernel module\n");
  printf("- Ensure that you run with administrative (super-user) privileges\n");
  printf("- Enabling/disabling a control type is an optional feature not supported by all control types\n");
  printf("- Resetting a zone energy counter is an optional powercap feature not supported by all control types\n");
}

int main(int argc, char** argv) {
  const char* control_type = NULL;
  u32_param ct_enabled = {0, 0};
  uint32_t zones[MAX_ZONE_DEPTH] = { 0 };
  uint32_t depth = 0;
  u32_param constraint = {0, 0};
  int reset_energy = 0;
  u32_param enabled = {0, 0};
  u64_param power_limit = {0, 0};
  u64_param time_window = {0, 0};
  int is_set_zone = 0;
  int is_set_constraint = 0;
  int c;
  int cont = 1;
  int ret = 0;

  /* Parse command-line arguments */
  while (cont) {
    c = getopt_long(argc, argv, short_options, long_options, NULL);
    switch (c) {
    case -1:
      cont = 0;
      break;
    case 'h':
      print_usage();
      return EXIT_SUCCESS;
    case 1:
    case 'p':
      if (control_type) {
        cont = 0;
        ret = -EINVAL;
      }
      control_type = optarg;
      break;
    case 'E':
      ret = set_u32_param(&ct_enabled, optarg, &cont);
      break;
    case 'z':
      ret = parse_zones(optarg, zones, MAX_ZONE_DEPTH, &depth, &cont);
      break;
    case 'c':
      ret = set_u32_param(&constraint, optarg, &cont);
      break;
    case 'j':
      if (reset_energy) {
        cont = 0;
        ret = -EINVAL;
      }
      reset_energy = 1;
      is_set_zone = 1;
      break;
    case 'e':
      ret = set_u32_param(&enabled, optarg, &cont);
      is_set_zone = 1;
      break;
    case 'l':
      ret = set_u64_param(&power_limit, optarg, &cont);
      is_set_constraint = 1;
      break;
    case 's':
      ret = set_u64_param(&time_window, optarg, &cont);
      is_set_constraint = 1;
      break;
    case '?':
    default:
      cont = 0;
      ret = -EINVAL;
      break;
    }
  }

  /* Verify argument combinations */
  if (ret) {
    fprintf(stderr, "Invalid arguments\n");
  } else if (!is_valid_powercap_control_type(control_type)) {
    fprintf(stderr, "Must specify control type NAME; value must not be empty or contain any '.' or '/' characters\n");
    ret = -EINVAL;
  } else if (!depth && (is_set_zone || is_set_constraint)) {
    fprintf(stderr, "Must specify -z/--zone with zone-level or constraint-level argument\n");
    ret = -EINVAL;
  } else if (depth && !(is_set_zone || is_set_constraint)) {
    fprintf(stderr, "Must specify zone-level or constraint-level argument with -z/--zone\n");
    ret = -EINVAL;
  } else if (!constraint.set && is_set_constraint) {
    fprintf(stderr, "Must specify -c/--constraint with constraint-level argument\n");
    ret = -EINVAL;
  } else if (constraint.set && !is_set_constraint) {
    fprintf(stderr, "Must specify constraint-level argument with -c/--constraint\n");
    ret = -EINVAL;
  } else if (!ct_enabled.set && !is_set_zone && !is_set_constraint) {
    fprintf(stderr, "Nothing to do\n");
    ret = -EINVAL;
  }
  if (ret) {
    print_usage();
    return EXIT_FAILURE;
  }

  /* Check if control type/zones/constraint exist */
  if (powercap_sysfs_control_type_exists(control_type)) {
    fprintf(stderr, "Control type does not exist\n");
    ret = -EINVAL;
  } else if (depth && powercap_sysfs_zone_exists(control_type, zones, depth)) {
    fprintf(stderr, "Zone does not exist\n");
    ret = -EINVAL;
  } else if (constraint.set && powercap_sysfs_constraint_exists(control_type, zones, depth, constraint.val)) {
    fprintf(stderr, "Constraint does not exist\n");
    ret = -EINVAL;
  }
  if (ret) {
    print_common_help();
    return EXIT_FAILURE;
  }

  /* Perform requested action(s) */
  if (ct_enabled.set) {
    c = powercap_sysfs_control_type_set_enabled(control_type, ct_enabled.val);
    if (c) {
      perror("Error setting control type enabled/disabled");
      ret |= c;
    }
  }
  if (reset_energy) {
    c = powercap_sysfs_zone_reset_energy_uj(control_type, zones, depth);
    if (c) {
      perror("Error setting zone energy counter");
      ret |= c;
    }
  }
  if (enabled.set) {
    c = powercap_sysfs_zone_set_enabled(control_type, zones, depth, enabled.val);
    if (c) {
      perror("Error setting zone enabled/disabled");
      ret |= c;
    }
  }
  if (power_limit.set) {
    c = powercap_sysfs_constraint_set_power_limit_uw(control_type, zones, depth, constraint.val, power_limit.val);
    if (c) {
      perror("Error setting constraint power limit");
      ret |= c;
    }
  }
  if (time_window.set) {
    c = powercap_sysfs_constraint_set_time_window_us(control_type, zones, depth, constraint.val, time_window.val);
    if (c) {
      perror("Error setting constraint time window");
      ret |= c;
    }
  }
  if (ret) {
    print_common_help();
  }

  return ret ? EXIT_FAILURE : EXIT_SUCCESS;
}
