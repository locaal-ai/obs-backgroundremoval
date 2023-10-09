#include "obs-config-utils.h"
#include "plugin-support.h"

#include <obs-module.h>
#include <util/config-file.h>
#include <filesystem>

void create_config_folder()
{
    char *config_folder_path = obs_module_config_path("");
    if (config_folder_path == nullptr) {
        obs_log(LOG_ERROR, "Failed to get config folder path");
        return;
    }
    std::filesystem::path config_folder_std_path(config_folder_path);
    bfree(config_folder_path);

    // create the folder if it doesn't exist
	if (!std::filesystem::exists(config_folder_std_path)) {
		obs_log(LOG_INFO, "Config folder does not exist, creating: %S", config_folder_std_path.c_str());
		// Create the config folder
		std::filesystem::create_directories(config_folder_std_path);
	}
}

int getFlagFromConfig(const char *name, bool *returnValue, bool defaultValue)
{
    create_config_folder(); // ensure the config folder exists

    // Get the config file
	char *config_file_path = obs_module_config_path("config.ini");

    config_t *config;
    int ret = config_open(&config, config_file_path, CONFIG_OPEN_EXISTING);
    if (ret != CONFIG_SUCCESS) {
        obs_log(LOG_INFO, "Failed to open config file %s", config_file_path);
        *returnValue = defaultValue;
        return OBS_BGREMOVAL_CONFIG_FAIL;
    }

    *returnValue = config_get_bool(config, "config", name);
    config_close(config);

    bfree(config_file_path);

	return OBS_BGREMOVAL_CONFIG_SUCCESS;
}

int setFlagFromConfig(const char *name, const bool value)
{
    create_config_folder(); // ensure the config folder exists

	// Get the config file
	char *config_file_path = obs_module_config_path("config.ini");

    config_t *config;
    int ret = config_open(&config, config_file_path, CONFIG_OPEN_ALWAYS);
    if (ret != CONFIG_SUCCESS) {
        obs_log(LOG_INFO, "Failed to open config file %s", config_file_path);
        return OBS_BGREMOVAL_CONFIG_FAIL;
    }

    config_set_bool(config, "config", name, value);
    config_save(config);
    config_close(config);

    bfree(config_file_path);

	return OBS_BGREMOVAL_CONFIG_SUCCESS;
}
