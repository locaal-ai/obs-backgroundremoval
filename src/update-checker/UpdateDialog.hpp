#include <QtWidgets>

class UpdateDialog : public QDialog {
	Q_OBJECT
public:
	UpdateDialog(const char* latestVersion, QWidget *parent = nullptr);

private:
	QVBoxLayout *layout;
	void disableUpdateChecks(int state);
};
