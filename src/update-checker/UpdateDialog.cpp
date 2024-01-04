#include "UpdateDialog.hpp"
#include "obs-utils/obs-config-utils.h"
#include "plugin-support.h"

#include <obs.h>
#include <obs-module.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QString>

static QString dialogContent =
	"<h1>Background Removal - Update available! 🚀</h1>"
	"<p>A new version of the Background Removal plugin (<a "
	"href=\"https://github.com/occ-ai/obs-backgroundremoval/releases\">v{version}</a>) is "
	"now available for <a "
	"href=\"https://github.com/occ-ai/obs-backgroundremoval/releases\">download</a>.</p>"
	"<p>If you have any questions, requests or need assistance, feel free to reach out"
	" to our <a href=\"https://github.com/occ-ai/obs-backgroundremoval/issues\">support team</a>.</p>"
	"<p>Thank you for using our plugin - enjoy the latest release! 🙏</p>"
	"<h2>Changelog</h2>";

UpdateDialog::UpdateDialog(
	struct github_utils_release_information latestVersion, QWidget *parent)
	: QDialog(parent), layout(new QVBoxLayout)
{
	setWindowTitle("Background Removal - Update available! 🚀");
	setLayout(layout);
	QLabel *label = new QLabel(dialogContent.replace(
		QString("{version}"),
		QString::fromStdString(latestVersion.version)));
	label->setOpenExternalLinks(true);
	label->setTextInteractionFlags(Qt::TextBrowserInteraction);
	label->setTextFormat(Qt::RichText);
	label->setWordWrap(true);
	layout->addWidget(label);

	QScrollArea *scrollArea = new QScrollArea;
	QLabel *scrollAreaLabel =
		new QLabel(QString::fromStdString(latestVersion.responseBody));
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
		setFlagInConfig("check_for_updates", state == Qt::Unchecked);
	});

	// Add a button to close the dialog
	QPushButton *closeButton = new QPushButton("Close");
	layout->addWidget(closeButton);
	connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
}
