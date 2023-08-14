#include "update-checker.h"
#include "UpdateDialog.hpp"
#include "github-utils.h"
#include "obs-utils/obs-config-utils.h"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <plugin-support.h>

#include <QTimer>

UpdateDialog *update_dialog;

extern "C" const char *PLUGIN_VERSION;

void check_update(void)
{
	github_utils_get_release_information([](github_utils_release_information
							info) {
		if (info.responseCode == OBS_BGREMOVAL_GITHUB_UTILS_SUCCESS) {
			obs_log(LOG_INFO, "Latest release is %s", info.version);
			bool shouldCheckForUpdates = false;
			if (getFlagFromConfig("check_for_updates",
					      &shouldCheckForUpdates) !=
			    OBS_BGREMOVAL_CONFIG_SUCCESS) {
				// Failed to get the config value, assume it's enabled
				shouldCheckForUpdates = true;
			}

			if (!shouldCheckForUpdates) {
				// Update checks are disabled
				return;
			}

			if (strcmp(info.version, PLUGIN_VERSION) == 0) {
				// No update available, latest version is the same as the current version
				return;
			}

			update_dialog = new UpdateDialog(
				info,
				(QWidget *)obs_frontend_get_main_window());
			QTimer::singleShot(2000, update_dialog,
					   &UpdateDialog::exec);
		} else {
			obs_log(LOG_INFO,
				"failed to get latest release information");
		}
		github_utils_release_information_free(info);
	});
}
