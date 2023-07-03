#include <QtWidgets>

class UpdateDialog : public QDialog {
	Q_OBJECT
public:
	UpdateDialog(QWidget *parent = nullptr);

private:
	QVBoxLayout *layout;
};
