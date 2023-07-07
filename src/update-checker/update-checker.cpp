#include "update-checker.h"
#include "UpdateDialog.hpp"
#include "github-utils.h"
#include "obs-utils/obs-config-utils.h"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QTimer>

UpdateDialog *update_dialog;

extern "C" const char *PLUGIN_VERSION;

void check_update(struct github_utils_release_information latestRelease)
{
	bool shouldCheckForUpdates = false;
	if (getFlagFromConfig("check_for_updates", &shouldCheckForUpdates) !=
	    OBS_BGREMOVAL_CONFIG_SUCCESS) {
		// Failed to get the config value, assume it's enabled
		shouldCheckForUpdates = true;
	}

	if (!shouldCheckForUpdates) {
		// Update checks are disabled
		return;
	}

	if (strcmp(latestRelease.version, PLUGIN_VERSION) == 0) {
		// No update available, latest version is the same as the current version
		return;
	}

	update_dialog = new UpdateDialog(
		latestRelease, (QWidget *)obs_frontend_get_main_window());
	QTimer::singleShot(2000, update_dialog, &UpdateDialog::exec);
}
