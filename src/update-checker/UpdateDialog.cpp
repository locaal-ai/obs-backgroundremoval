#include "UpdateDialog.hpp"
#include "obs-utils/obs-config-utils.h"

#include <obs.h>
#include <obs-module.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QString>

static QString dialogContent =
	"<h1>Background Removal - Update available! 🚀</h1>"
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
	"<p>Thank you for using our plugin and we hope you enjoy the latest release! 🙏</p>"
	"<h2>Changelog</h2>";

UpdateDialog::UpdateDialog(
	struct github_utils_release_information latestVersion, QWidget *parent)
	: QDialog(parent), layout(new QVBoxLayout)
{
	setWindowTitle("Background Removal - Update available! 🚀");
	setLayout(layout);
	QLabel *label = new QLabel(dialogContent.replace(
		QString("{version}"), QString(latestVersion.version)));
	label->setOpenExternalLinks(true);
	label->setTextInteractionFlags(Qt::TextBrowserInteraction);
	label->setTextFormat(Qt::RichText);
	label->setWordWrap(true);
	layout->addWidget(label);

	QScrollArea *scrollArea = new QScrollArea;
	QLabel *scrollAreaLabel =
		new QLabel(QString(latestVersion.responseBody));
	scrollAreaLabel->setOpenExternalLinks(true);
	scrollAreaLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
	scrollAreaLabel->setTextFormat(Qt::MarkdownText);
	scrollAreaLabel->setWordWrap(true);
	scrollArea->setWidget(scrollAreaLabel);
	scrollArea->setWidgetResizable(true);
	layout->addWidget(scrollArea);

	// Add a checkbox to disable update checks
	QCheckBox *disableCheckbox = new QCheckBox("Disable update checks");
	layout->addWidget(disableCheckbox);
	connect(disableCheckbox, &QCheckBox::stateChanged, this, [](int state) {
		setFlagFromConfig("check_for_updates", state == Qt::Unchecked);
	});

	// Add a button to close the dialog
	QPushButton *closeButton = new QPushButton("Close");
	layout->addWidget(closeButton);
	connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
}
