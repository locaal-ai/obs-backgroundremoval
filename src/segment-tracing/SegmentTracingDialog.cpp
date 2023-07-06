#include "SegmentTracingDialog.h"
#include "obs-utils/obs-config-utils.h"
#include "segment-tracing.h"
#include "plugin-support.h"

#include <obs.h>
#include <obs-module.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QString>

static QString dialogContent =
	"<h1>Help us improve Background Removal!</h1>"
	"<p>We're always looking for ways to improve our plugin and we'd like to ask you to opt-in to our "
	"anonymous usage statistics collection. This will help us understand how the plugin is being used "
	"and what features we should focus on next.</p>"
	"<p>If you are willing to help us, please check the box below to opt-in. If you'd rather not "
	"participate, you can simply close this dialog.</p>"
	"<p><b>Privacy notice:</b> We will only collect anonymous, aggregate usage statistics and will "
	"never collect any personal information.</p>"
	"<p>Example of the anonymous information we collect:</p>"
	"<ul>"
	"<li>Number of times the plugin was loaded, used, activated, updated</li>"
	"<li>Number of times each feature was used, such as the model and blur option</li>"
	"<li>The operating system and OBS version</li>"
	"</ul>"
	"<p>Your information will be processed and stored securely and privately on AWS servers in the "
	"United States. By clicking the button below, you agree to this processing and storage. For "
	"additional</p>"
	"<p>The anonymous information will only be accessible to the Background Removal team "
	"and will never be shared with any third parties. <i>Aggregate</i> statistics may be shared "
	"publicly in the future.</p>"
	"<p>To know exactly what information we collect and how, please review the code "
	"<a href=\"https://github.com/royshil/obs-backgroundremoval\">here</a>.</p>"
	"<p><b>Thank you for helping us improve Background Removal!</b> 🙏</p>"
	"<p>If you have any questions or concerns, please feel free to reach out to our <a "
	"href=\"https://github.com/royshil/obs-backgroundremoval/issues\">support team</a>.</p>";

SegmentTracingDialog::SegmentTracingDialog(QWidget *parent)
	: QDialog(parent), layout(new QVBoxLayout)
{
	setWindowTitle("Background Removal - Usage Statistics Collection 📊");
	setLayout(layout);
	QLabel *label = new QLabel(dialogContent);
	label->setOpenExternalLinks(true);
	label->setTextInteractionFlags(Qt::TextBrowserInteraction);
	label->setTextFormat(Qt::RichText);
	label->setWordWrap(true);

	QScrollArea *scrollArea = new QScrollArea;
	scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidget(label);
	layout->addWidget(scrollArea);

	// Add a checkbox
	segmentTracingEnableCheckbox =
		new QCheckBox("Collect usage statistics");
	segmentTracingEnableCheckbox->setCheckState(Qt::Checked);
	layout->addWidget(segmentTracingEnableCheckbox);
	connect(segmentTracingEnableCheckbox, &QCheckBox::stateChanged, this,
		[](int state) {
			setFlagFromConfig("segment_tracing",
					  state == Qt::Checked);
		});

	startupCheckbox = new QCheckBox("Show this dialog on startup");
	startupCheckbox->setCheckState(Qt::Unchecked);
	connect(startupCheckbox, &QCheckBox::stateChanged, this, [](int state) {
		setFlagFromConfig("show_segment_tracing_dialog",
				  state == Qt::Checked);
	});
	layout->addWidget(startupCheckbox);

	// Add a button to close the dialog
	QPushButton *closeButton = new QPushButton("Close");
	layout->addWidget(closeButton);
	connect(closeButton, &QPushButton::clicked, this,
		&SegmentTracingDialog::closeDialog);
}

void SegmentTracingDialog::closeDialog()
{
	// Save config value for showing the dialog on startup
	setFlagFromConfig("show_segment_tracing_dialog",
			  startupCheckbox->checkState() == Qt::Checked);
	// Save config value for segment tracing
	setFlagFromConfig("segment_tracing",
			  segmentTracingEnableCheckbox->checkState() ==
				  Qt::Checked);

	if (segmentTracingEnableCheckbox->checkState() == Qt::Checked) {
		segment_tracing_init_tracing();
	} else {
		segment_tracing_deinit();
	}

	// Close the dialog
	QDialog::close();
}
