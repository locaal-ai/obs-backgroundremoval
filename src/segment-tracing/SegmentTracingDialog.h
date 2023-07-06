#ifndef SEGMENTTRACINGDIALOG_H
#define SEGMENTTRACINGDIALOG_H

#include <QtWidgets>

class SegmentTracingDialog : public QDialog {
	Q_OBJECT
public:
	SegmentTracingDialog(QWidget *parent = nullptr);

private:
	QVBoxLayout *layout;
	QCheckBox *segmentTracingEnableCheckbox;
	QCheckBox *startupCheckbox;
	void closeDialog();
};

#endif /* SEGMENTTRACINGDIALOG_H */
