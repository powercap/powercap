/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Get powercap values.
 *
 * @author Connor Imes
 * @date 2017-08-24
 */
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "powercap-sysfs.h"
#include "util-common.h"

#define POWERCAP_PATH "/sys/class/powercap"

static void print_parent_headers(const uint32_t* zones, uint32_t depth_start, uint32_t depth, uint32_t indnt) {
  uint32_t i;
  uint32_t j;
  for (i = depth_start; i <= depth; i++) {
    indent(indnt + i - 1);
    printf("Zone %"PRIu32, zones[0]);
    for (j = 1; j < i; j++) {
      printf(":%"PRIu32, zones[j]);
    }
    printf("\n");
  }
}

static void analyze_constraint(const char* control_type, const uint32_t* zones, uint32_t depth, uint32_t constraint, int verbose, uint32_t indnt) {
  char name[MAX_NAME_SIZE];
  uint64_t val64;
  ssize_t sret;
  int ret;

  indent(indnt + depth);
  printf("Constraint %"PRIu32"\n", constraint);

  sret = powercap_sysfs_constraint_get_name(control_type, zones, depth, constraint, name, sizeof(name));
  ret = sret > 0 ? 0 : (int) sret;
  str_or_verbose(verbose, indnt + depth + 1, "name", name, ret);

  ret = powercap_sysfs_constraint_get_power_limit_uw(control_type, zones, depth, constraint, &val64);
  u64_or_verbose(verbose, indnt + depth + 1, "power_limit_uw", val64, ret);

  ret = powercap_sysfs_constraint_get_time_window_us(control_type, zones, depth, constraint, &val64);
  u64_or_verbose(verbose, indnt + depth + 1, "time_window_us", val64, ret);

  ret = powercap_sysfs_constraint_get_min_power_uw(control_type, zones, depth, constraint, &val64);
  u64_or_verbose(verbose, indnt + depth + 1, "min_power_uw", val64, ret);

  ret = powercap_sysfs_constraint_get_max_power_uw(control_type, zones, depth, constraint, &val64);
  u64_or_verbose(verbose, indnt + depth + 1, "max_power_uw", val64, ret);

  ret = powercap_sysfs_constraint_get_min_time_window_us(control_type, zones, depth, constraint, &val64);
  u64_or_verbose(verbose, indnt + depth + 1, "min_time_window_us", val64, ret);

  ret = powercap_sysfs_constraint_get_max_time_window_us(control_type, zones, depth, constraint, &val64);
  u64_or_verbose(verbose, indnt + depth + 1, "max_time_window_us", val64, ret);
}

static void analyze_zone(const char* control_type, const uint32_t* zones, uint32_t depth, int verbose, uint32_t indnt) {
  char name[MAX_NAME_SIZE];
  uint64_t val64;
  uint32_t val32;
  ssize_t sret;
  int ret;

  print_parent_headers(zones, depth, depth, indnt);

  sret = powercap_sysfs_zone_get_name(control_type, zones, depth, name, sizeof(name));
  ret = sret > 0 ? 0 : (int) sret;
  str_or_verbose(verbose, indnt + depth, "name", name, ret);

  ret = powercap_sysfs_zone_get_enabled(control_type, zones, depth, &val32);
  u64_or_verbose(verbose, indnt + depth, "enabled", (uint64_t) val32, ret);

  ret = powercap_sysfs_zone_get_max_energy_range_uj(control_type, zones, depth, &val64);
  u64_or_verbose(verbose, indnt + depth, "max_energy_range_uj", val64, ret);

  ret = powercap_sysfs_zone_get_energy_uj(control_type, zones, depth, &val64);
  u64_or_verbose(verbose, indnt + depth, "energy_uj", val64, ret);

  ret = powercap_sysfs_zone_get_max_power_range_uw(control_type, zones, depth, &val64);
  u64_or_verbose(verbose, indnt + depth, "max_power_range_uw", val64, ret);

  ret = powercap_sysfs_zone_get_power_uw(control_type, zones, depth, &val64);
  u64_or_verbose(verbose, indnt + depth, "power_uw", val64, ret);

  for (val32 = 0; !powercap_sysfs_constraint_exists(control_type, zones, depth, val32); val32++) {
    analyze_constraint(control_type, zones, depth, val32, verbose, indnt);
  }
}

static void analyze_control_type(const char* control_type, int verbose, uint32_t indnt) {
  uint32_t val32;
  int ret = powercap_sysfs_control_type_get_enabled(control_type, &val32);
  u64_or_verbose(verbose, indnt, "enabled", (uint64_t) val32, ret);
}

/* depth must be > 0 */
static void analyze_all_zones_recurse(const char* control_type, uint32_t* zones, uint32_t depth, uint32_t max_depth, int verbose, uint32_t indnt) {
  if (!powercap_sysfs_zone_exists(control_type, zones, depth)) {
    /* Analyze this zone */
    analyze_zone(control_type, zones, depth, verbose, indnt);
    if (depth < max_depth) {
      /* Analyze subzones */
      zones[depth] = 0;
      analyze_all_zones_recurse(control_type, zones, depth + 1, max_depth, verbose, indnt);
    }
    /* Analyze next sibling zone */
    zones[depth - 1]++;
    analyze_all_zones_recurse(control_type, zones, depth, max_depth, verbose, indnt);
  }
}

static void analyze_control_type_recurse(const char* control_type, uint32_t* zones, uint32_t max_depth, int verbose, uint32_t indnt) {
  analyze_control_type(control_type, verbose, indnt);
  analyze_all_zones_recurse(control_type, zones, 1, max_depth, verbose, indnt);
}

static int analyze_powercap(uint32_t* zones, uint32_t max_depth, int verbose) {
  struct dirent *dp;
  DIR *dfd;
  if ((dfd = opendir(POWERCAP_PATH)) == NULL) {
    perror(POWERCAP_PATH);
    return -errno;
  }
  while ((dp = readdir(dfd)) != NULL) {
    if (!strstr(dp->d_name, ".") && !strstr(dp->d_name, ":")) {
      memset(zones, 0, max_depth * sizeof(*zones));
      printf("%s\n", dp->d_name);
      analyze_control_type_recurse(dp->d_name, zones, max_depth, verbose, 1);
    }
  }
  if (closedir(dfd)) {
    perror(POWERCAP_PATH);
    return -errno;
  }
  return 0;
}

static void analyze_zone_recurse(const char* control_type, uint32_t* zones, uint32_t depth, uint32_t max_depth, int verbose, uint32_t indnt) {
  if (!powercap_sysfs_zone_exists(control_type, zones, depth)) {
    /* Analyze this zone */
    analyze_zone(control_type, zones, depth, verbose, indnt);
    if (depth < max_depth) {
      /* Analyze subzones */
      zones[depth] = 0;
      analyze_all_zones_recurse(control_type, zones, depth + 1, max_depth, verbose, indnt);
    }
  }
}

static void print_num_zones(const char* control_type, uint32_t* zones, uint32_t depth) {
  zones[depth] = 0;
  while (!powercap_sysfs_zone_exists(control_type, zones, depth + 1)) {
    zones[depth]++;
  }
  printf("%"PRIu32"\n", zones[depth]);
}

static void print_num_constraints(const char* control_type, uint32_t* zones, uint32_t depth) {
  uint32_t n = 0;
  while (!powercap_sysfs_constraint_exists(control_type, zones, depth, n)) {
    n++;
  }
  printf("%"PRIu32"\n", n);
}

static const char short_options[] = "-hvp:z:c:EnNjJwWexlsUuTty";

static const struct option long_options[] = {
  {"help",                no_argument,        NULL, 'h'},
  {"verbose",             no_argument,        NULL, 'v'},
  {"control-type",        required_argument,  NULL, 'p'},
  {"zone",                required_argument,  NULL, 'z'},
  {"constraint",          required_argument,  NULL, 'c'},
  {"enabled",             no_argument,        NULL, 'E'},
  {"nzones",              no_argument,        NULL, 'n'},
  {"nconstraints",        no_argument,        NULL, 'N'},
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
  printf("Usage: powercap-info [NAME [OPTION]...] [-hv]\n\n");
  printf("Prints configurations for powercap control types.\n");
  printf("Output can be filtered by specifying a control type NAME and OPTION flags.\n");
  printf("A control type NAME must not be empty or contain a '.' or '/'.\n\n");
  printf("Options:\n");
  printf("  -h, --help                   Print this message and exit\n");
  printf("  -v, --verbose                Print errors when files cannot be read\n");
  printf("  -p, --control-type=NAME      Deprecated, provide NAME as the first\n");
  printf("                               positional argument instead\n");
  printf("All remaining options below require a control type NAME:\n");
  printf("  -z, --zone=ZONE(S)           The zone/subzone numbers in the control type's\n");
  printf("                               powercap tree (control type's root by default)\n");
  printf("                               Separate zones/subzones with a colon\n");
  printf("                               E.g., for zone 0, subzone 2: \"-z 0:2\"\n");
  printf("                               Ending with a colon prevents output for subzones\n");
  printf("                               E.g., for zone 0, but not subzones: \"-z 0:\"\n");
  printf("  -c, --constraint=CONSTRAINT  The constraint number\n");
  printf("All remaining options below are mutually exclusive:\n");
  printf("  -E, --enabled                Print control type enabled/disabled status\n");
  printf("  -n, --nzones                 Print the number of zones (control type's root by\n");
  printf("                               default; within the -z/--zone level, if set)\n");
  printf("The following are zone-level arguments and require -z/--zone:\n");
  printf("  -N, --nconstraints           Print the number of zone constraints\n");
  printf("  -j, --z-energy               Print zone energy counter\n");
  printf("  -J, --z-max-energy-range     Print zone maximum energy counter range\n");
  printf("  -w, --z-power                Print zone current power\n");
  printf("  -W, --z-max-power-range      Print zone maximum current power range\n");
  printf("  -e, --z-enabled              Print zone enabled/disabled status\n");
  printf("  -x, --z-name                 Print zone name\n");
  printf("The following are constraint-level arguments and require -z/--zone and -c/--constraint:\n");
  printf("  -l, --c-power-limit          Print constraint power limit\n");
  printf("  -s, --c-time-window          Print constraint time window\n");
  printf("  -U, --c-max-power            Print constraint maximum allowed power\n");
  printf("  -u, --c-min-power            Print constraint minimum allowed power\n");
  printf("  -T, --c-max-time-window      Print constraint maximum allowed time window\n");
  printf("  -t, --c-min-time-window      Print constraint minimum allowed time window\n");
  printf("  -y, --c-name                 Print constraint name\n");
  printf("\nSome fields are optional and/or may require administrative (super-user) privileges to read.\n");
  printf("Fields will only be printed if they are available and readable, unless -v/--verbose is set.\n");
  printf("If no zone/constraint-specific outputs are requested, all available zones and constraints will be shown.\n");
  printf("\nEnergy units: microjoules (uJ)\n");
  printf("Power units: microwatts (uW)\n");
  printf("Time units: microseconds (us)\n");
}

static void print_common_help(void) {
  printf("Considerations for common errors:\n");
  printf("- Ensure that the control type exists, which may require loading a kernel module\n");
  printf("- Some files may simply not exist\n");
}

static int verify_control_type_args(const char* control_type, uint32_t depth, u32_param* constraint, int unique_set) {
  int ret = 0;
  if (!is_valid_powercap_control_type(control_type)) {
    fprintf(stderr, "Must specify control type NAME; value must not be empty or contain any '.' or '/' characters\n");
    ret = -EINVAL;
  } else if (unique_set) {
    switch (unique_set) {
    case 'E':
      if (depth || constraint->set) {
        fprintf(stderr, "Must not specify -z/--zone or -c/--constraint with -E/--enabled\n");
        ret = -EINVAL;
      }
      break;
    case 'n':
      if (constraint->set) {
        fprintf(stderr, "Must not specify -c/--constraint with -n/--nzones\n");
        ret = -EINVAL;
      }
      break;
    case 'N':
    case 'j':
    case 'J':
    case 'w':
    case 'W':
    case 'e':
    case 'x':
      if (!depth) {
        fprintf(stderr, "Must specify -z/--zone with zone-level argument\n");
        ret = -EINVAL;
      } else if (constraint->set) {
        fprintf(stderr, "Must not specify -c/--constraint with zone-level argument\n");
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
      if (!depth || !constraint->set) {
        fprintf(stderr, "Must specify -z/--zone and -c/--constraint with constraint-level argument\n");
        ret = -EINVAL;
      }
      break;
    }
  } else if (constraint->set && !depth) {
    fprintf(stderr, "Must specify -z/--zone with -c/--constraint\n");
    ret = -EINVAL;
  }
  return ret;
}

static int print_control_type(const char* control_type, uint32_t* zones, uint32_t depth, uint32_t max_depth,
                              u32_param* constraint, int recurse, int verbose, int unique_set) {
  uint64_t val64;
  uint32_t val32;
  char name[MAX_NAME_SIZE];
  int ret = 0;

  /* Check if control type/zones/constraint exist */
  if (powercap_sysfs_control_type_exists(control_type)) {
    fprintf(stderr, "Control type does not exist\n");
    return -EINVAL;
  } else if (depth && powercap_sysfs_zone_exists(control_type, zones, depth)) {
    fprintf(stderr, "Zone does not exist\n");
    return -EINVAL;
  } else if (constraint->set && powercap_sysfs_constraint_exists(control_type, zones, depth, constraint->val)) {
    fprintf(stderr, "Constraint does not exist\n");
    return -EINVAL;
  }

  /* Perform requested action */
  if (unique_set) {
    switch (unique_set) {
    case 'E':
      /* Get control type enabled */
      if (!(ret = powercap_sysfs_control_type_get_enabled(control_type, &val32))) {
        printf("%"PRIu32"\n", val32);
      } else {
        perror("Failed to get control type enabled");
      }
      break;
    case 'n':
      /* Print number of zones at the specified tree location */
      print_num_zones(control_type, zones, depth);
      break;
    case 'N':
      /* Print number of constraints at the specified tree location */
      print_num_constraints(control_type, zones, depth);
      break;
    case 'j':
      /* Get zone energy */
      if (!(ret = powercap_sysfs_zone_get_energy_uj(control_type, zones, depth, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get zone energy");
      }
      break;
    case 'J':
      /* Get zone max energy range */
      if (!(ret = powercap_sysfs_zone_get_max_energy_range_uj(control_type, zones, depth, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get zone max energy range");
      }
      break;
    case 'w':
      /* Get zone power */
      if (!(ret = powercap_sysfs_zone_get_power_uw(control_type, zones, depth, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get zone power");
      }
      break;
    case 'W':
      /* Get zone max power range */
      if (!(ret = powercap_sysfs_zone_get_max_power_range_uw(control_type, zones, depth, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get zone max power range");
      }
      break;
    case 'e':
      /* Get zone enabled */
      if (!(ret = powercap_sysfs_zone_get_enabled(control_type, zones, depth, &val32))) {
        printf("%"PRIu32"\n", val32);
      } else {
        perror("Failed to get zone enabled");
      }
      break;
    case 'x':
      /* Get zone name */
      if (powercap_sysfs_zone_get_name(control_type, zones, depth, name, sizeof(name)) > 0) {
        printf("%s\n", name);
      } else {
        ret = -errno;
        perror("Failed to get zone name");
      }
      break;
    case 'l':
      /* Get constraint power limit */
      if (!(ret = powercap_sysfs_constraint_get_power_limit_uw(control_type, zones, depth, constraint->val, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get constraint power limit");
      }
      break;
    case 's':
      /* Get constraint time window */
      if (!(ret = powercap_sysfs_constraint_get_time_window_us(control_type, zones, depth, constraint->val, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get constraint time window");
      }
      break;
    case 'U':
      /* Get constraint max power */
      if (!(ret = powercap_sysfs_constraint_get_max_power_uw(control_type, zones, depth, constraint->val, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get constraint max power");
      }
      break;
    case 'u':
      /* Get constraint min power */
      if (!(ret = powercap_sysfs_constraint_get_min_power_uw(control_type, zones, depth, constraint->val, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get constraint min power");
      }
      break;
    case 'T':
      /* Get constraint max time window */
      if (!(ret = powercap_sysfs_constraint_get_max_time_window_us(control_type, zones, depth, constraint->val, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get constraint max time window");
      }
      break;
    case 't':
      /* Get constraint min time window */
      if (!(ret = powercap_sysfs_constraint_get_min_time_window_us(control_type, zones, depth, constraint->val, &val64))) {
        printf("%"PRIu64"\n", val64);
      } else {
        perror("Failed to get constraint min time window");
      }
      break;
    case 'y':
      /* Get constraint name */
      if (powercap_sysfs_constraint_get_name(control_type, zones, depth, constraint->val, name, sizeof(name)) > 0) {
        printf("%s\n", name);
      } else {
        ret = -errno;
        perror("Failed to get constraint name");
      }
      break;
    }
  } else if (depth > 0) {
    /* Print summary of zone or constraint */
    if (constraint->set) {
      /* print constraint */
      print_parent_headers(zones, 1, depth, 0);
      analyze_constraint(control_type, zones, depth, constraint->val, verbose, 0);
    } else {
      /* print zone */
      print_parent_headers(zones, 1, depth - 1, 0);
      if (recurse) {
        analyze_zone_recurse(control_type, zones, depth, max_depth, verbose, 0);
      } else {
        analyze_zone(control_type, zones, depth, verbose, 0);
      }
    }
  } else {
    /* print control type and all zones */
    analyze_control_type_recurse(control_type, zones, max_depth, verbose, 0);
  }
  return ret;
}

int main(int argc, char** argv) {
  const char* control_type = NULL;
  uint32_t zones[MAX_ZONE_DEPTH] = { 0 };
  u32_param constraint = {0, 0};
  uint32_t depth = 0;
  int recurse = 1;
  int verbose = 0;
  int unique_set = 0;
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
    case 'v':
      verbose = 1;
      break;
    case 1:
    case 'p':
      if (control_type) {
        cont = 0;
        ret = -EINVAL;
      }
      control_type = optarg;
      break;
    case 'z':
      recurse = get_recurse(optarg);
      ret = parse_zones(optarg, zones, MAX_ZONE_DEPTH, &depth, &cont);
      break;
    case 'c':
      ret = set_u32_param(&constraint, optarg, &cont);
      break;
    case 'E':
    case 'n':
    case 'N':
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
        fprintf(stderr, "Must not specify multiple mutually exclusive arguments\n");
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
    fprintf(stderr, "Invalid arguments\n");
  } else if (control_type) {
    ret = verify_control_type_args(control_type, depth, &constraint, unique_set);
  } else if (depth || constraint.set || unique_set) {
    fprintf(stderr, "Must not specify control type options without NAME\n");
    ret = -EINVAL;
  }
  if (ret) {
    print_usage();
    return EXIT_FAILURE;
  }

  /* Print requested info */
  if (control_type) {
    ret = print_control_type(control_type, zones, depth, MAX_ZONE_DEPTH, &constraint, recurse, verbose, unique_set);
  } else {
    ret = analyze_powercap(zones, MAX_ZONE_DEPTH, verbose);
  }
  if (ret) {
    print_common_help();
  }

  return ret ? EXIT_FAILURE : EXIT_SUCCESS;
}
