#ifndef OBS_CONFIG_UTILS_H
#define OBS_CONFIG_UTILS_H

enum {
	OBS_BGREMOVAL_CONFIG_SUCCESS = 0,
	OBS_BGREMOVAL_CONFIG_FAIL = 1,
};

/**
 * Get a boolean flasg from the module configuration file.
 *
 * @param name The name of the config item.
 * @param returnValue The value of the config item.
 * @param defaultValue The default value of the config item.
 * @return OBS_BGREMOVAL_CONFIG_SUCCESS if the config item was found,
 * OBS_BGREMOVAL_CONFIG_FAIL otherwise.
 */
int getFlagFromConfig(const char *name, bool *returnValue, bool defaultValue);

/**
 * Set a boolean flag in the module configuration file.
 *
 * @param name The name of the config item.
 * @param value The value of the config item.
 * @return OBS_BGREMOVAL_CONFIG_SUCCESS if the config item was found,
 * OBS_BGREMOVAL_CONFIG_FAIL otherwise.
 */
int setFlagInConfig(const char *name, const bool value);

#endif /* OBS_CONFIG_UTILS_H */
