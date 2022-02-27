/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Get RAPL values.
 *
 * @author Connor Imes
 * @date 2017-08-24
 */
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "powercap-rapl-sysfs.h"
#include "util-common.h"

static void print_headers(uint32_t zone, uint32_t do_zone, uint32_t sz, int is_sz) {
  if (do_zone) {
    printf("Zone %"PRIu32"\n", zone);
  }
  if (is_sz) {
    indent(1);
    printf("Subzone %"PRIu32"\n", sz);
  }
}

static void analyze_constraint(uint32_t zone, uint32_t sz, int is_sz, uint32_t constraint, int verbose) {
  char name[MAX_NAME_SIZE];
  uint64_t val64;
  ssize_t sret;
  int ret;

  indent((uint32_t) is_sz + 1);
  printf("Constraint %"PRIu32"\n", constraint);

  sret = rapl_sysfs_constraint_get_name(zone, sz, is_sz, constraint, name, sizeof(name));
  ret = sret > 0 ? 0 : (int) sret;
  str_or_verbose(verbose, (uint32_t) is_sz + 2, "name", name, ret);

  ret = rapl_sysfs_constraint_get_power_limit_uw(zone, sz, is_sz, constraint, &val64);
  u64_or_verbose(verbose, (uint32_t) is_sz + 2, "power_limit_uw", val64, ret);

  ret = rapl_sysfs_constraint_get_time_window_us(zone, sz, is_sz, constraint, &val64);
  u64_or_verbose(verbose, (uint32_t) is_sz + 2, "time_window_us", val64, ret);

  ret = rapl_sysfs_constraint_get_min_power_uw(zone, sz, is_sz, constraint, &val64);
  u64_or_verbose(verbose, (uint32_t) is_sz + 2, "min_power_uw", val64, ret);

  ret = rapl_sysfs_constraint_get_max_power_uw(zone, sz, is_sz, constraint, &val64);
  u64_or_verbose(verbose, (uint32_t) is_sz + 2, "max_power_uw", val64, ret);

  ret = rapl_sysfs_constraint_get_min_time_window_us(zone, sz, is_sz, constraint, &val64);
  u64_or_verbose(verbose, (uint32_t) is_sz + 2, "min_time_window_us", val64, ret);

  ret = rapl_sysfs_constraint_get_max_time_window_us(zone, sz, is_sz, constraint, &val64);
  u64_or_verbose(verbose, (uint32_t) is_sz + 2, "max_time_window_us", val64, ret);
}

static void analyze_zone(uint32_t zone, uint32_t sz, int is_sz, int verbose) {
  char name[MAX_NAME_SIZE];
  uint64_t val64;
  uint32_t val32;
  ssize_t sret;
  int ret;

  print_headers(0, 0, sz, is_sz);

  sret = rapl_sysfs_zone_get_name(zone, sz, is_sz, name, sizeof(name));
  ret = sret > 0 ? 0 : (int) sret;
  str_or_verbose(verbose, (uint32_t) is_sz + 1, "name", name, ret);

  ret = rapl_sysfs_zone_get_enabled(zone, sz, is_sz, &val32);
  u64_or_verbose(verbose, (uint32_t) is_sz + 1, "enabled", (uint64_t) val32, ret);

  ret = rapl_sysfs_zone_get_max_energy_range_uj(zone, sz, is_sz, &val64);
  u64_or_verbose(verbose, (uint32_t) is_sz + 1, "max_energy_range_uj", val64, ret);

  ret = rapl_sysfs_zone_get_energy_uj(zone, sz, is_sz, &val64);
  u64_or_verbose(verbose, (uint32_t) is_sz + 1, "energy_uj", val64, ret);

  ret = rapl_sysfs_zone_get_max_power_range_uw(zone, sz, is_sz, &val64);
  u64_or_verbose(verbose, (uint32_t) is_sz + 1, "max_power_range_uw", val64, ret);

  ret = rapl_sysfs_zone_get_power_uw(zone, sz, is_sz, &val64);
  u64_or_verbose(verbose, (uint32_t) is_sz + 1, "power_uw", val64, ret);

  for (val32 = 0; !rapl_sysfs_constraint_exists(zone, sz, is_sz, val32); val32++) {
    analyze_constraint(zone, sz, is_sz, val32, verbose);
  }
}

static void analyze_zone_recurse(uint32_t zone, int verbose) {
  uint32_t sz;
  print_headers(zone, 1, 0, 0);
  analyze_zone(zone, 0, 0, verbose);
  for (sz = 0; !rapl_sysfs_zone_exists(zone, sz, 1); sz++) {
    analyze_zone(zone, sz, 1, verbose);
  }
}

static void analyze_all_zones_recurse(int verbose) {
  uint32_t zone;
  for (zone = 0; !rapl_sysfs_zone_exists(zone, 0, 0); zone++) {
    analyze_zone_recurse(zone, verbose);
  }
}

static void print_num_zones(void) {
  uint32_t n = 0;
  while (!rapl_sysfs_zone_exists(n, 0, 0)) {
    n++;
  }
  printf("%"PRIu32"\n", n);
}

static void print_num_subzones(uint32_t zone) {
  uint32_t n = 0;
  while (!rapl_sysfs_zone_exists(zone, n, 1)) {
    n++;
  }
  printf("%"PRIu32"\n", n);
}

static const char short_options[] = "hvp:z:c:njJwWexlsUuTty";
static const struct option long_options[] = {
  {"help",                no_argument,        NULL, 'h'},
  {"verbose",             no_argument,        NULL, 'v'},
  {"package",             required_argument,  NULL, 'p'},
  {"zone",                required_argument,  NULL, 'p'},
  {"subzone",             required_argument,  NULL, 'z'},
  {"constraint",          required_argument,  NULL, 'c'},
  {"nzones",              no_argument,        NULL, 'n'},
  {"z-energy",            no_argument,        NULL, 'j'},
  {"z-max-energy-range",  no_argument,        NULL, 'J'},
  {"z-power",             no_argument,        NULL, 'w'},
  {"z-max-power-range",   no_argument,        NULL, 'W'},
  {"z-enabled",           no_argument,        NULL, 'e'},
  {"z-name",              no_argument,        NULL, 'x'},
  {"c-power-limit",       no_argument,        NULL, 'l'},
  {"c-time-window",       no_argument,        NULL, 's'},
  {"c-max-power",         no_argument,        NULL, 'U'},
  {"c-min-power",         no_argument,        NULL, 'u'},
  {"c-max-time-window",   no_argument,        NULL, 'T'},
  {"c-min-time-window",   no_argument,        NULL, 't'},
  {"c-name",              no_argument,        NULL, 'y'},
  {0, 0, 0, 0}
};

static void print_usage(void) {
  printf("\nThis utility is deprecated, use powercap-info instead.\n\n");
  printf("Usage: rapl-info [OPTION]...\n");
  printf("Options:\n");
  printf("  -h, --help                   Print this message and exit\n");
  printf("  -v, --verbose                Print errors when files are not available\n");
  printf("  -p, --zone=ZONE              The zone number (none by default; 0 by default if\n");
  printf("                               using -z/--subzone and/or -c/--constraint)\n");
  printf("                               Ending with a colon prevents output for subzones\n");
  printf("                               E.g., for zone 0, but not subzones: \"-p 0:\"\n");
  printf("      --package=PACKAGE        Deprecated, use --zone instead\n");
  printf("  -z, --subzone=SUBZONE        The subzone number (none by default)\n");
  printf("  -c, --constraint=CONSTRAINT  The constraint number (none by default)\n");
  printf("All remaining options below are mutually exclusive:\n");
  printf("  -n, --nzones                 Print the number of zones found, or the number of\n");
  printf("                               subzones found if -p/--zone is set\n");
  printf("The following are zone-level arguments (-z/--subzone is optional):\n");
  printf("  -j, --z-energy               Print zone energy counter\n");
  printf("  -J, --z-max-energy-range     Print zone maximum energy counter range\n");
  printf("  -w, --z-power                Print zone current power\n");
  printf("  -W, --z-max-power-range      Print zone maximum current power range\n");
  printf("  -e, --z-enabled              Print zone enabled/disabled status\n");
  printf("  -x, --z-name                 Print zone name\n");
  printf("The following are constraint-level arguments and require -c/--constraint (-z/--subzone is optional):\n");
  printf("  -l, --c-power-limit          Print constraint power limit\n");
  printf("  -s, --c-time-window          Print constraint time window\n");
  printf("  -U, --c-max-power            Print constraint maximum allowed power\n");
  printf("  -u, --c-min-power            Print constraint minimum allowed power\n");
  printf("  -T, --c-max-time-window      Print constraint maximum allowed time window\n");
  printf("  -t, --c-min-time-window      Print constraint minimum allowed time window\n");
  printf("  -y, --c-name                 Print constraint name\n");
  printf("\nSome fields are optional and will only be printed if they are available unless -v/--verbose is set.\n");
  printf("If no zone/constraint-specific outputs are requested, all available zones and constraints will be shown.\n");
  printf("\nEnergy units: microjoules (uJ)\n");
  printf("Power units: microwatts (uW)\n");
  printf("Time units: microseconds (us)\n");
}

static void print_common_help(void) {
  printf("Considerations for common errors:\n");
  printf("- Ensure that the intel_rapl kernel module is loaded\n");
  printf("- Some files may simply not exist\n");
  printf("- On some systems, the kernel always returns an error when reading constraint max power (-U/--c-max-power)\n");
}

int main(int argc, char** argv) {
  u32_param zone = {0, 0};
  u32_param subzone = {0, 0};
  u32_param constraint = {0, 0};
  int recurse = 1;
  int verbose = 0;
  int unique_set = 0;
  int c;
  int cont = 1;
  int ret = 0;
  uint64_t val64;
  uint32_t val32;
  char name[MAX_NAME_SIZE];

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
    case 'v':
      verbose = 1;
      break;
    case 'p':
      recurse = get_recurse(optarg);
      ret = set_u32_param(&zone, optarg, &cont);
      break;
    case 'z':
      ret = set_u32_param(&subzone, optarg, &cont);
      break;
    case 'c':
      ret = set_u32_param(&constraint, optarg, &cont);
      break;
    case 'n':
    case 'j':
    case 'J':
    case 'w':
    case 'W':
    case 'e':
    case 'x':
    case 'l':
    case 's':
    case 'U':
    case 'u':
    case 'T':
    case 't':
    case 'y':
      if (unique_set) {
        fprintf(stderr, "Only one of -n/--nzones, a zone-level argument, or a constraint-level argument is allowed at a time\n");
        cont = 0;
        ret = -EINVAL;
        break;
      }
      unique_set = c;
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
    fprintf(stderr, "Unknown or duplicate arguments\n");
  } else {
    switch (unique_set) {
    case 'n':
      if (subzone.set || constraint.set) {
        fprintf(stderr, "-n/--nzones cannot be used with -z/--subzone or -c/--constraint\n");
        ret = -EINVAL;
      }
      break;
    case 'j':
    case 'J':
    case 'w':
    case 'W':
    case 'e':
    case 'x':
      if (constraint.set) {
        fprintf(stderr, "-c/--constraint cannot be set for zone-level arguments\n");
        ret = -EINVAL;
      }
      break;
    case 'l':
    case 's':
    case 'U':
    case 'u':
    case 'T':
    case 't':
    case 'y':
      if (!constraint.set) {
        fprintf(stderr, "-c/--constraint must be set for constraint-level arguments\n");
        ret = -EINVAL;
      }
      break;
    }
  }
  if (ret) {
    print_usage();
    return EXIT_FAILURE;
  }

  /* Check if zone/subzone/constraint exist */
  if (rapl_sysfs_zone_exists(zone.val, 0, 0)) {
    fprintf(stderr, "Zone does not exist\n");
    ret = -EINVAL;
  } else if (subzone.set && rapl_sysfs_zone_exists(zone.val, subzone.val, 1)) {
    fprintf(stderr, "Subzone does not exist\n");
    ret = -EINVAL;
  } else if (constraint.set && rapl_sysfs_constraint_exists(zone.val, subzone.val, subzone.set, constraint.val)) {
    fprintf(stderr, "Constraint does not exist\n");
    ret = -EINVAL;
  }
  if (ret) {
    print_common_help();
    return EXIT_FAILURE;
  }

  /* Perform requested action */
  if (unique_set) {
    switch (unique_set) {
    case 'n':
      /* Print number of zones or subzones */
      if (zone.set) {
        print_num_subzones(zone.val);
      } else {
        print_num_zones();
      }
      break;
    case 'j':
      /* Get zone energy */
      if (!(ret = rapl_sysfs_zone_get_energy_uj(zone.val, subzone.val, subzone.set, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get zone energy");
      }
      break;
    case 'J':
      /* Get zone max energy range */
      if (!(ret = rapl_sysfs_zone_get_max_energy_range_uj(zone.val, subzone.val, subzone.set, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get zone max energy range");
      }
      break;
    case 'w':
      /* Get zone power */
      if (!(ret = rapl_sysfs_zone_get_power_uw(zone.val, subzone.val, subzone.set, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get zone power");
      }
      break;
    case 'W':
      /* Get zone max power range */
      if (!(ret = rapl_sysfs_zone_get_max_power_range_uw(zone.val, subzone.val, subzone.set, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get zone max power range");
      }
      break;
    case 'e':
      /* Get zone enabled */
      if (!(ret = rapl_sysfs_zone_get_enabled(zone.val, subzone.val, subzone.set, &val32))) {
        printf("%"PRIu32"\n", val32);
      } else {
        perror("Failed to get zone enabled");
      }
      break;
    case 'x':
      /* Get zone name */
      if (rapl_sysfs_zone_get_name(zone.val, subzone.val, subzone.set, name, sizeof(name)) > 0) {
        printf("%s\n", name);
      } else {
        ret = -errno;
        perror("Failed to get zone name");
      }
      break;
    case 'l':
      /* Get constraint power limit */
      if (!(ret = rapl_sysfs_constraint_get_power_limit_uw(zone.val, subzone.val, subzone.set, constraint.val, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get constraint power limit");
      }
      break;
    case 's':
      /* Get constraint time window */
      if (!(ret = rapl_sysfs_constraint_get_time_window_us(zone.val, subzone.val, subzone.set, constraint.val, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get constraint time window");
      }
      break;
    case 'U':
      /* Get constraint max power */
      if (!(ret = rapl_sysfs_constraint_get_max_power_uw(zone.val, subzone.val, subzone.set, constraint.val, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get constraint max power");
      }
      break;
    case 'u':
      /* Get constraint min power */
      if (!(ret = rapl_sysfs_constraint_get_min_power_uw(zone.val, subzone.val, subzone.set, constraint.val, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get constraint min power");
      }
      break;
    case 'T':
      /* Get constraint max time window */
      if (!(ret = rapl_sysfs_constraint_get_max_time_window_us(zone.val, subzone.val, subzone.set, constraint.val, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get constraint max time window");
      }
      break;
    case 't':
      /* Get constraint min time window */
      if (!(ret = rapl_sysfs_constraint_get_min_time_window_us(zone.val, subzone.val, subzone.set, constraint.val, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get constraint min time window");
      }
      break;
    case 'y':
      /* Get constraint name */
      if (rapl_sysfs_constraint_get_name(zone.val, subzone.val, subzone.set, constraint.val, name, sizeof(name)) > 0) {
        printf("%s\n", name);
      } else {
        ret = -errno;
        perror("Failed to get constraint name");
      }
      break;
    }
  } else if (zone.set || subzone.set || constraint.set) {
    /* Print summary of zone, subzone, or constraint */
    if (constraint.set) {
      /* print constraint */
      print_headers(zone.val, 1, subzone.val, subzone.set);
      analyze_constraint(zone.val, subzone.val, subzone.set, constraint.val, verbose);
    } else if (subzone.set) {
      /* print subzone */
      print_headers(zone.val, 1, 0, 0);
      analyze_zone(zone.val, subzone.val, subzone.set, verbose);
    } else {
      /* print zone */
      if (recurse) {
        analyze_zone_recurse(zone.val, verbose);
      } else {
        analyze_zone(zone.val, 0, 0, verbose);
      }
    }
  } else {
    /* print all zones */
    if ((ret = rapl_sysfs_zone_exists(0, 0, 0))) {
      fprintf(stderr, "No RAPL zones found\n");
    } else {
      analyze_all_zones_recurse(verbose);
    }
  }
  if (ret) {
    print_common_help();
  }

  return ret ? EXIT_FAILURE : EXIT_SUCCESS;
}
