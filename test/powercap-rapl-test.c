/**
 * Basic tests - gets caps and sets them right back.
 */
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "powercap-rapl.h"

static const powercap_rapl_zone ZONES[] = { POWERCAP_RAPL_ZONE_PACKAGE, POWERCAP_RAPL_ZONE_CORE, POWERCAP_RAPL_ZONE_UNCORE, POWERCAP_RAPL_ZONE_DRAM, POWERCAP_RAPL_ZONE_PSYS };
static const uint32_t NZONES = 5;
static const char* const ZONE_NAMES[] = { "Package", "Core", "Uncore", "DRAM", "PSys" };
static const powercap_rapl_constraint CONSTRAINTS[] = { POWERCAP_RAPL_CONSTRAINT_LONG, POWERCAP_RAPL_CONSTRAINT_SHORT };
static const uint32_t NCONSTRAINTS = 2;

// optional parameter - boolean to enable read/write
int main(int argc, char** argv) {
  uint32_t i, j;
  int ret = 0;
  int ro = 1;
  if (argc > 1) {
    // a value other than 0 enables read/write
    ro = !atoi(argv[1]);
  }

  // initialize
  uint32_t npackages = powercap_rapl_get_num_packages();
  if (npackages == 0) {
    if (errno) {
      perror("powercap_rapl_get_num_packages");
    } else {
      fprintf(stderr, "No RAPL packages found\n");
    }
    return EXIT_FAILURE;
  }

  powercap_rapl_pkg* pkgs = malloc(npackages * sizeof(powercap_rapl_pkg));
  if (pkgs == NULL) {
    perror("malloc");
    return EXIT_FAILURE;
  }

  for (i = 0; i < npackages; i++) {
    if (powercap_rapl_init(i, &pkgs[i], ro)) {
      perror("powercap_rapl_init");
      ret = EXIT_FAILURE;
      npackages = i; // for correct cleanup count
      goto cleanup;
    }
  }
  printf("Initialized %"PRIu32" package(s)\n", npackages);

  // test functionality for all zones for a single package
  powercap_rapl_pkg* p = &pkgs[0];
  for (i = 0; i < NZONES; i++) {
    int supported;
    char name[32];
    ssize_t name_ret;
    int enabled;
    uint64_t val;

    supported = powercap_rapl_is_zone_supported(p, ZONES[i]);
    if (supported < 0) {
      perror("powercap_rapl_is_zone_supported");
      ret = EXIT_FAILURE;
      goto cleanup;
    } else if (supported == 0) {
      printf("Zone %s: not supported\n", ZONE_NAMES[i]);
      continue;
    }

    supported = powercap_rapl_is_zone_file_supported(p, ZONES[i], POWERCAP_ZONE_FILE_NAME);
    if (supported < 0) {
      perror("powercap_rapl_is_zone_file_supported");
      ret = EXIT_FAILURE;
      goto cleanup;
    } else if (supported == 0) {
      printf("%s name: not supported\n", ZONE_NAMES[i]);
    } else {
      name_ret = powercap_rapl_get_name(p, ZONES[i], name, sizeof(name));
      if (name_ret < 0) {
        perror("powercap_rapl_get_name");
        ret = EXIT_FAILURE;
        goto cleanup;
      }
      printf("%s name: %s\n", ZONE_NAMES[i], name_ret > 0 ? name : "[None]");
    }

    supported = powercap_rapl_is_zone_file_supported(p, ZONES[i], POWERCAP_ZONE_FILE_ENABLED);
    if (supported < 0) {
      perror("powercap_rapl_is_zone_file_supported");
      ret = EXIT_FAILURE;
      goto cleanup;
    } else if (supported == 0) {
      printf("%s enabled: not supported\n", ZONE_NAMES[i]);
    } else {
      enabled = powercap_rapl_is_enabled(p, ZONES[i]);
      if (enabled < 0) {
        perror("powercap_rapl_is_enabled");
        ret = EXIT_FAILURE;
        goto cleanup;
      }
      printf("%s enabled: %s\n", ZONE_NAMES[i], enabled > 0 ? "yes" : "no");
      if (!ro && powercap_rapl_set_enabled(p, ZONES[i], enabled)) {
        perror("powercap_rapl_set_enabled");
        ret = EXIT_FAILURE;
        goto cleanup;
      }
    }

    supported = powercap_rapl_is_zone_file_supported(p, ZONES[i], POWERCAP_ZONE_FILE_MAX_ENERGY_RANGE_UJ);
    if (supported < 0) {
      perror("powercap_rapl_is_zone_file_supported");
      ret = EXIT_FAILURE;
      goto cleanup;
    } else if (supported == 0) {
      printf("%s max_energy_range_uj: not supported\n", ZONE_NAMES[i]);
    } else {
      if (powercap_rapl_get_max_energy_range_uj(p, ZONES[i], &val)) {
        perror("powercap_rapl_get_max_energy_range_uj");
        ret = EXIT_FAILURE;
        goto cleanup;
      }
      printf("%s max_energy_range_uj: %"PRIu64"\n", ZONE_NAMES[i], val);
    }

    supported = powercap_rapl_is_zone_file_supported(p, ZONES[i], POWERCAP_ZONE_FILE_ENERGY_UJ);
    if (supported < 0) {
      perror("powercap_rapl_is_zone_file_supported");
      ret = EXIT_FAILURE;
      goto cleanup;
    } else if (supported == 0) {
      printf("%s energy_uj: not supported\n", ZONE_NAMES[i]);
    } else {
      if (powercap_rapl_get_energy_uj(p, ZONES[i], &val)) {
        perror("powercap_rapl_get_energy_uj");
        ret = EXIT_FAILURE;
        goto cleanup;
      }
      printf("%s energy_uj: %"PRIu64"\n", ZONE_NAMES[i], val);
      // TODO: Test powercap_rapl_reset_energy_uj
    }

    supported = powercap_rapl_is_zone_file_supported(p, ZONES[i], POWERCAP_ZONE_FILE_MAX_POWER_RANGE_UW);
    if (supported < 0) {
      perror("powercap_rapl_is_zone_file_supported");
      ret = EXIT_FAILURE;
      goto cleanup;
    } else if (supported == 0) {
      printf("%s max_power_range_uw: not supported\n", ZONE_NAMES[i]);
    } else {
      if (powercap_rapl_get_max_power_range_uw(p, ZONES[i], &val)) {
        perror("powercap_rapl_get_max_power_range_uw");
        ret = EXIT_FAILURE;
        goto cleanup;
      }
      printf("%s max_power_range_uw: %"PRIu64"\n", ZONE_NAMES[i], val);
    }

    supported = powercap_rapl_is_zone_file_supported(p, ZONES[i], POWERCAP_ZONE_FILE_POWER_UW);
    if (supported < 0) {
      perror("powercap_rapl_is_zone_file_supported");
      ret = EXIT_FAILURE;
      goto cleanup;
    } else if (supported == 0) {
      printf("%s power_uw: not supported\n", ZONE_NAMES[i]);
    } else {
      if (powercap_rapl_get_power_uw(p, ZONES[i], &val)) {
        perror("powercap_rapl_get_power_uw");
        ret = EXIT_FAILURE;
        goto cleanup;
      }
      printf("%s power_uw: %"PRIu64"\n", ZONE_NAMES[i], val);
    }

    // test long and short term constraint properties
    for (j = 0; j < NCONSTRAINTS; j++) {
      const char* const cnst = CONSTRAINTS[j] == POWERCAP_RAPL_CONSTRAINT_LONG ? "long" : "short";

      supported = powercap_rapl_is_constraint_supported(p, ZONES[i], CONSTRAINTS[j]);
      if (supported < 0) {
        perror("powercap_rapl_is_constraint_supported");
        ret = EXIT_FAILURE;
        goto cleanup;
      } else if (supported == 0) {
        printf("%s constraint_(%s): not supported\n", ZONE_NAMES[i], cnst);
        continue;
      }

      supported = powercap_rapl_is_constraint_file_supported(p, ZONES[i], CONSTRAINTS[j], POWERCAP_CONSTRAINT_FILE_MAX_POWER_UW);
      if (supported < 0) {
        perror("powercap_rapl_is_constraint_file_supported");
        ret = EXIT_FAILURE;
        goto cleanup;
      } else if (supported == 0) {
        printf("%s constraint_(%s)_max_power_uw: not supported\n", ZONE_NAMES[i], cnst);
      } else {
        if (powercap_rapl_get_max_power_uw(p, ZONES[i], CONSTRAINTS[j], &val)) {
          perror("powercap_rapl_get_max_power_uw");
          // TODO: powercap_rapl_get_max_power_uw fails and sets errno=ENODATA for power planes on development system...
          // ret = EXIT_FAILURE;
          // goto cleanup;
        } else {
          printf("%s constraint_(%s)_max_power_uw: %"PRIu64"\n", ZONE_NAMES[i], cnst, val);
        }
      }

      supported = powercap_rapl_is_constraint_file_supported(p, ZONES[i], CONSTRAINTS[j], POWERCAP_CONSTRAINT_FILE_MIN_POWER_UW);
      if (supported < 0) {
        perror("powercap_rapl_is_constraint_file_supported");
        ret = EXIT_FAILURE;
        goto cleanup;
      } else if (supported == 0) {
        printf("%s constraint_(%s)_min_power_uw: not supported\n", ZONE_NAMES[i], cnst);
      } else {
        if (powercap_rapl_get_min_power_uw(p, ZONES[i], CONSTRAINTS[j], &val)) {
          perror("powercap_rapl_get_min_power_uw");
          ret = EXIT_FAILURE;
          goto cleanup;
        } else {
          printf("%s constraint_(%s)_min_power_uw: %"PRIu64"\n", ZONE_NAMES[i], cnst, val);
        }
      }

      supported = powercap_rapl_is_constraint_file_supported(p, ZONES[i], CONSTRAINTS[j], POWERCAP_CONSTRAINT_FILE_POWER_LIMIT_UW);
      if (supported < 0) {
        perror("powercap_rapl_is_constraint_file_supported");
        ret = EXIT_FAILURE;
        goto cleanup;
      } else if (supported == 0) {
        printf("%s constraint_(%s)_power_limit_uw: not supported\n", ZONE_NAMES[i], cnst);
      } else {
        if (powercap_rapl_get_power_limit_uw(p, ZONES[i], CONSTRAINTS[j], &val)) {
          perror("powercap_rapl_get_power_limit_uw");
          ret = EXIT_FAILURE;
          goto cleanup;
        } else {
          printf("%s constraint_(%s)_power_limit_uw: %"PRIu64"\n", ZONE_NAMES[i], cnst, val);
          if (!ro && powercap_rapl_set_power_limit_uw(p, ZONES[i], CONSTRAINTS[j], val)) {
            perror("powercap_rapl_set_power_limit_uw");
            ret = 1;
          }
        }
      }

      supported = powercap_rapl_is_constraint_file_supported(p, ZONES[i], CONSTRAINTS[j], POWERCAP_CONSTRAINT_FILE_MAX_TIME_WINDOW_US);
      if (supported < 0) {
        perror("powercap_rapl_is_constraint_file_supported");
        ret = EXIT_FAILURE;
        goto cleanup;
      } else if (supported == 0) {
        printf("%s constraint_(%s)_max_time_window_us: not supported\n", ZONE_NAMES[i], cnst);
      } else {
        if (powercap_rapl_get_max_time_window_us(p, ZONES[i], CONSTRAINTS[j], &val)) {
          perror("powercap_rapl_get_max_time_window_us");
          ret = EXIT_FAILURE;
          goto cleanup;
        } else {
          printf("%s constraint_(%s)_max_time_window_us: %"PRIu64"\n", ZONE_NAMES[i], cnst, val);
        }
      }

      supported = powercap_rapl_is_constraint_file_supported(p, ZONES[i], CONSTRAINTS[j], POWERCAP_CONSTRAINT_FILE_MIN_TIME_WINDOW_US);
      if (supported < 0) {
        perror("powercap_rapl_is_constraint_file_supported");
        ret = EXIT_FAILURE;
        goto cleanup;
      } else if (supported == 0) {
        printf("%s constraint_(%s)_min_time_window_us: not supported\n", ZONE_NAMES[i], cnst);
      } else {
        if (powercap_rapl_get_min_time_window_us(p, ZONES[i], CONSTRAINTS[j], &val)) {
          perror("powercap_rapl_get_min_time_window_us");
          ret = EXIT_FAILURE;
          goto cleanup;
        } else {
          printf("%s constraint_(%s)_min_time_window_us: %"PRIu64"\n", ZONE_NAMES[i], cnst, val);
        }
      }

      supported = powercap_rapl_is_constraint_file_supported(p, ZONES[i], CONSTRAINTS[j], POWERCAP_CONSTRAINT_FILE_TIME_WINDOW_US);
      if (supported < 0) {
        perror("powercap_rapl_is_constraint_file_supported");
        ret = EXIT_FAILURE;
        goto cleanup;
      } else if (supported == 0) {
        printf("%s constraint_(%s)_time_window_us: not supported\n", ZONE_NAMES[i], cnst);
      } else {
        if (powercap_rapl_get_time_window_us(p, ZONES[i], CONSTRAINTS[j], &val)) {
          perror("powercap_rapl_get_time_window_us");
          ret = EXIT_FAILURE;
          goto cleanup;
        } else {
          printf("%s constraint_(%s)_time_window_us: %"PRIu64"\n", ZONE_NAMES[i], cnst, val);
          if (!ro && powercap_rapl_set_time_window_us(p, ZONES[i], CONSTRAINTS[j], val)) {
            perror("powercap_rapl_set_time_window_us");
            ret = 1;
          }
        }
      }

      supported = powercap_rapl_is_constraint_file_supported(p, ZONES[i], CONSTRAINTS[j], POWERCAP_CONSTRAINT_FILE_NAME);
      if (supported < 0) {
        perror("powercap_rapl_is_constraint_file_supported");
        ret = EXIT_FAILURE;
        goto cleanup;
      } else if (supported == 0) {
        printf("%s constraint_(%s)_name: not supported\n", ZONE_NAMES[i], cnst);
      } else {
        name_ret = powercap_rapl_get_constraint_name(p, ZONES[i], CONSTRAINTS[j], name, sizeof(name));
        if (name_ret < 0) {
          perror("powercap_rapl_get_constraint_name");
          ret = EXIT_FAILURE;
          goto cleanup;
        }
        printf("%s constraint_(%s)_name: %s\n", ZONE_NAMES[i], cnst, name_ret > 0 ? name : "[None]");
        }
    }
  }

cleanup:
  for (i = 0; i < npackages; i++) {
    if (powercap_rapl_destroy(&pkgs[i])) {
      perror("powercap_rapl_destroy");
      ret = EXIT_FAILURE;
    }
  }
  free(pkgs);
  printf("Cleaned up\n");

  return ret;
}
