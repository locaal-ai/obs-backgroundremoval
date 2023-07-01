#include <QTimer>

#include <obs-frontend-api.h>

#include "update-checker.h"

#include "UpdateDialog.hpp"

UpdateDialog *update_dialog;

void check_update(void)
{
	update_dialog =
		new UpdateDialog((QWidget *)obs_frontend_get_main_window());
	QTimer::singleShot(2000, update_dialog, &UpdateDialog::exec);
}
