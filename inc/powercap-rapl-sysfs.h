/**
 * Read/write RAPL sysfs files.
 * This is a wrapper around powercap-sysfs.h.
 *
 * The control type is "intel-rapl" and zone depth is limited to 2.
 * The "pkg" parameters below are for the top-level "zone", and the optional "sz" parameters are for other control
 * planes like "core", "uncore", and "dram".
 * The "is_sz" parameter must be non-zero when working with these control planes.
 *
 * For example, pkg=0, sz=0, is_sz=1 is usually for the "core" power plane and is analogous to using powercap-sysfs.h
 * with zones[2]={0, 0}, depth=2.
 *
 * @author Connor Imes
 * @date 2017-08-24
 */

#ifndef _RAPL_SYSFS_H
#define _RAPL_SYSFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <unistd.h>

/**
 * Determine if a package exists.
 *
 * @param pkg
 * @return 0 if package exists, -ENOSYS otherwise.
 */
int rapl_sysfs_pkg_exists(uint32_t pkg);

/**
 * Determine if a subzone exists.
 *
 * @param pkg
 * @param sz
 * @return 0 if subzone exists, -ENOSYS otherwise.
 */
int rapl_sysfs_sz_exists(uint32_t pkg, uint32_t sz);

/**
 * Determine if a constraint exists.
 *
 * @param pkg
 * @param sz
 * @param is_sz
 * @param constraint
 * @return 0 if constraint exists, -ENOSYS otherwise.
 */
int rapl_sysfs_constraint_exists(uint32_t pkg, uint32_t sz, int is_sz, uint32_t constraint);

/**
 * Get max_energy_range_uj for a zone.
 *
 * @param pkg
 * @param sz
 * @param is_sz
 * @param val
 * @return 0 on success, a negative error code otherwise.
 */
int rapl_sysfs_zone_get_max_energy_range_uj(uint32_t pkg, uint32_t sz, int is_sz, uint64_t* val);

/**
 * Get energy_uj for a zone.
 *
 * @param pkg
 * @param sz
 * @param is_sz
 * @param val
 * @return 0 on success, a negative error code otherwise.
 */
int rapl_sysfs_zone_get_energy_uj(uint32_t pkg, uint32_t sz, int is_sz, uint64_t* val);

/**
 * Enable/disable a zone.
 *
 * @param pkg
 * @param sz
 * @param is_sz
 * @param val
 * @return 0 on success, a negative error code otherwise.
 */
int rapl_sysfs_zone_set_enabled(uint32_t pkg, uint32_t sz, int is_sz, uint32_t val);

/**
 * Get whether a zone is enabled or disabled.
 *
 * @param pkg
 * @param sz
 * @param is_sz
 * @param val
 * @return 0 on success, a negative error code otherwise.
 */
int rapl_sysfs_zone_get_enabled(uint32_t pkg, uint32_t sz, int is_sz, uint32_t* val);

/**
 * Get name for a zone.
 *
 * @param pkg
 * @param sz
 * @param is_sz
 * @param buf
 * @param size
 * @return number of bytes read, a negative error code otherwise.
 */
ssize_t rapl_sysfs_zone_get_name(uint32_t pkg, uint32_t sz, int is_sz, char* buf, size_t size);

/**
 * Set power_limit_uw for a constraint.
 *
 * @param pkg
 * @param sz
 * @param is_sz
 * @param constraint
 * @param val
 * @return 0 on success, a negative error code otherwise.
 */
int rapl_sysfs_constraint_set_power_limit_uw(uint32_t pkg, uint32_t sz, int is_sz, uint32_t constraint, uint64_t val);

/**
 * Get power_limit_uw for a constraint.
 *
 * @param pkg
 * @param sz
 * @param is_sz
 * @param constraint
 * @param val
 * @return 0 on success, a negative error code otherwise.
 */
int rapl_sysfs_constraint_get_power_limit_uw(uint32_t pkg, uint32_t sz, int is_sz, uint32_t constraint, uint64_t* val);

/**
 * Set time_window_us for a constraint.
 *
 * @param pkg
 * @param sz
 * @param is_sz
 * @param constraint
 * @param val
 * @return 0 on success, a negative error code otherwise.
 */
int rapl_sysfs_constraint_set_time_window_us(uint32_t pkg, uint32_t sz, int is_sz, uint32_t constraint, uint64_t val);

/**
 * Get time_window_us for a constraint.
 *
 * @param pkg
 * @param sz
 * @param is_sz
 * @param constraint
 * @param val
 * @return 0 on success, a negative error code otherwise.
 */
int rapl_sysfs_constraint_get_time_window_us(uint32_t pkg, uint32_t sz, int is_sz, uint32_t constraint, uint64_t* val);

/**
 * Get max_power_uw for a constraint.
 *
 * @param pkg
 * @param sz
 * @param is_sz
 * @param constraint
 * @param val
 * @return 0 on success, a negative error code otherwise.
 */
int rapl_sysfs_constraint_get_max_power_uw(uint32_t pkg, uint32_t sz, int is_sz, uint32_t constraint, uint64_t* val);

/**
 * Get name for a constraint.
 *
 * @param pkg
 * @param sz
 * @param is_sz
 * @param constraint
 * @param buf
 * @param size
 * @return number of bytes read, a negative error code otherwise.
 */
ssize_t rapl_sysfs_constraint_get_name(uint32_t pkg, uint32_t sz, int is_sz, uint32_t constraint, char* buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif
