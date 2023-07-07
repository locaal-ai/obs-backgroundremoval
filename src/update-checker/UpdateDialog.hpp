#include <QtWidgets>

#include "github-utils.h"

class UpdateDialog : public QDialog {
	Q_OBJECT
public:
	UpdateDialog(struct github_utils_release_information latestVersion,
		     QWidget *parent = nullptr);

private:
	QVBoxLayout *layout;
};
