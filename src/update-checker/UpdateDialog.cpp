#include "UpdateDialog.hpp"

#include <obs.h>
#include <obs-module.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QString>

static QString dialogContent =
	"<p>A new version of the Background Removal plugin (<a "
	"href=\"https://github.com/royshil/obs-backgroundremoval/releases\">v{version}</a>) is "
	"now available for download. We've made some exciting updates and improvements that we think "
	"you'll love. To get the latest features and enhancements, please follow the link below:</p>"
	"<p>Download the latest version from GitHub: <a "
	"href=\"https://github.com/royshil/obs-backgroundremoval/releases\">v{version}</a></p>"
	"<p>Once you've downloaded the new version, install the update as usual, there's no need to "
	"uninstall the previous version.</p>"
	"<p>If you have any questions or need assistance during the update process, feel free to reach out"
	" to our <a href=\"https://github.com/royshil/obs-backgroundremoval/issues\">support team</a>.</p>"
	"<p>Thank you for using our plugin and we hope you enjoy the latest release! 🙏</p>";

UpdateDialog::UpdateDialog(const char *latestVersion, QWidget *parent)
	: QDialog(parent), layout(new QVBoxLayout)
{
	setWindowTitle("Background Removal - Update available! 🚀");
	setLayout(layout);
	QLabel *label = new QLabel(dialogContent.replace(
		QString("{version}"), QString(latestVersion)));
	label->setOpenExternalLinks(true);
	label->setTextInteractionFlags(Qt::TextBrowserInteraction);
	label->setTextFormat(Qt::RichText);
	label->setWordWrap(true);
	layout->addWidget(label);
	// Add a checkbox to disable update checks
	QCheckBox *disableCheckbox = new QCheckBox("Disable update checks");
	layout->addWidget(disableCheckbox);
	connect(disableCheckbox, &QCheckBox::stateChanged, this,
		&UpdateDialog::disableUpdateChecks);
	// Add a button to close the dialog
	QPushButton *closeButton = new QPushButton("Close");
	layout->addWidget(closeButton);
	connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
}

void UpdateDialog::disableUpdateChecks(int state)
{
	UNUSED_PARAMETER(state);

	// Get the config file
	char *config_file = obs_module_file("config.json");
	if (!config_file) {
		blog(LOG_INFO, "Unable to find config file");
		return;
	}

	// Parse the config file
	obs_data_t *data = obs_data_create_from_json_file(config_file);
	if (!data) {
		blog(LOG_INFO, "Failed to parse config file");
		return;
	}

	// Update the config
	obs_data_set_bool(data, "check_for_updates", state == Qt::Unchecked);
	obs_data_save_json(data, config_file);

	obs_data_release(data);
}
