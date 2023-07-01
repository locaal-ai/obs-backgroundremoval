#include "UpdateDialog.hpp"

UpdateDialog::UpdateDialog(QWidget *parent)
	: QDialog(parent), layout(new QVBoxLayout)
{
	setWindowTitle("Update available!");
	setLayout(layout);
	QLabel *label = new QLabel("OBS Background Removal: Update available!");
	layout->addWidget(label);
}
