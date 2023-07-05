#include <QTimer>

#include "update-checker.h"
#include "UpdateDialog.hpp"

#include <obs-frontend-api.h>

UpdateDialog *update_dialog;

extern "C" const char *PLUGIN_VERSION;

void check_update(const char* latestRelease)
{
	if (strcmp(latestRelease, PLUGIN_VERSION) == 0) {
		// No update available, latest version is the same as the current version
		return;
	}
	update_dialog =
		new UpdateDialog(latestRelease, (QWidget *)obs_frontend_get_main_window());
	QTimer::singleShot(2000, update_dialog, &UpdateDialog::exec);
}
