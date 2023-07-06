#include "update-checker.h"
#include "UpdateDialog.hpp"
#include "obs-utils/obs-config-utils.h"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QTimer>

UpdateDialog *update_dialog;

extern "C" const char *PLUGIN_VERSION;

void check_update(const char *latestRelease)
{
	// Check configuration to see if update checks are disabled
	bool shouldCheckForUpdates;
	if (getFlagFromConfig("checkForUpdates", &shouldCheckForUpdates) ==
	    OBS_BGREMOVAL_CONFIG_FAIL) {
		// Failed to get config, assume update checks are enabled
		shouldCheckForUpdates = true;
	}

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
