#include "update-checker.h"
#include "UpdateDialog.hpp"
#include "github-utils.h"
#include "obs-utils/obs-config-utils.h"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <plugin-support.h>

#include <QTimer>

extern "C" const char *PLUGIN_VERSION;

UpdateDialog *update_dialog = nullptr;

void check_update(void)
{
	bool shouldCheckForUpdates = false;
	if (getFlagFromConfig("check_for_updates", &shouldCheckForUpdates,
			      true) != OBS_BGREMOVAL_CONFIG_SUCCESS) {
		// Failed to get the config value, assume it's enabled
		shouldCheckForUpdates = true;
		// store the default value
		setFlagFromConfig("check_for_updates", shouldCheckForUpdates);
	}

	if (!shouldCheckForUpdates) {
		// Update checks are disabled
		return;
	}

	const auto callback = [](github_utils_release_information info) {
		if (info.responseCode != OBS_BGREMOVAL_GITHUB_UTILS_SUCCESS) {
			obs_log(LOG_INFO,
				"failed to get latest release information");
			return;
		}
		obs_log(LOG_INFO, "Latest release is %s", info.version.c_str());

		if (info.version == PLUGIN_VERSION) {
			// No update available, latest version is the same as the current version
			return;
		}

		try {
			QTimer::singleShot(2000, [=]() {
				QWidget *main_window = (QWidget *)
					obs_frontend_get_main_window();
				if (main_window == nullptr) {
					obs_log(LOG_ERROR,
						"Update Checker failed to get main window");
					return;
				}
				update_dialog =
					new UpdateDialog(info, main_window);
				update_dialog->exec();
			});
		} catch (...) {
			obs_log(LOG_ERROR, "Failed to construct UpdateDialog");
		}
	};

	github_utils_get_release_information(callback);
}
