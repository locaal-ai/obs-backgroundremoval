#include "obs-config-utils.h"
#include "plugin-support.h"

#include <obs-module.h>

int getFlagFromConfig(const char *name, bool *returnValue)
{
	// Check configuration to see if update checks are disabled
	char *config_file = obs_module_file("config.json");
	if (!config_file) {
		obs_log(LOG_INFO, "Unable to find config file");
		return OBS_BGREMOVAL_CONFIG_FAIL;
	}

	obs_data_t *data = obs_data_create_from_json_file(config_file);
	if (!data) {
		obs_log(LOG_INFO, "Failed to parse config file");
		return OBS_BGREMOVAL_CONFIG_FAIL;
	}

	*returnValue = obs_data_get_bool(data, name);
	obs_data_release(data);

	return OBS_BGREMOVAL_CONFIG_SUCCESS;
}

int setFlagFromConfig(const char *name, const bool value)
{
	// Get the config file
	char *config_file = obs_module_file("config.json");
	if (!config_file) {
		obs_log(LOG_INFO, "Unable to find config file");
		return OBS_BGREMOVAL_CONFIG_FAIL;
	}

	// Parse the config file
	obs_data_t *json_data = obs_data_create_from_json_file(config_file);
	if (!json_data) {
		obs_log(LOG_INFO, "Failed to parse config file");
		return OBS_BGREMOVAL_CONFIG_FAIL;
	}

	// Update the config
	obs_data_set_bool(json_data, name, value);
	obs_data_save_json(json_data, config_file);

	obs_data_release(json_data);

	return OBS_BGREMOVAL_CONFIG_SUCCESS;
}
