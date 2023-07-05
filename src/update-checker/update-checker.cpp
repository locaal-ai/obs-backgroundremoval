#include "update-checker.h"
#include "UpdateDialog.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QTimer>

UpdateDialog *update_dialog;

extern "C" const char *PLUGIN_VERSION;

void check_update(const char *latestRelease)
{
	// Check configuration to see if update checks are disabled
	char *config_file = obs_module_file("config.json");
	if (!config_file) {
		blog(LOG_INFO, "Unable to find config file");
		return;
	}

	obs_data_t *data = obs_data_create_from_json_file(config_file);
	if (!data) {
		blog(LOG_INFO, "Failed to parse config file");
		return;
	}

	bool shouldCheckForUpdates =
		obs_data_get_bool(data, "check_for_updates");
	obs_data_release(data);
	if (!shouldCheckForUpdates) {
		// Update checks are disabled
		return;
	}

	if (strcmp(latestRelease, PLUGIN_VERSION) == 0) {
		// No update available, latest version is the same as the current version
		return;
	}

	update_dialog = new UpdateDialog(
		latestRelease, (QWidget *)obs_frontend_get_main_window());
	QTimer::singleShot(2000, update_dialog, &UpdateDialog::exec);
}
