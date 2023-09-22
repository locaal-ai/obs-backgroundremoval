#include <QtWidgets>

#include <opencv2/core.hpp>

class MaskEditorDialog : public QDialog {
	Q_OBJECT
public:
	MaskEditorDialog(QWidget *parent, const cv::Mat& mask8UC1);
  ~MaskEditorDialog();

private:
	QVBoxLayout *layout;
  bool eventFilter(QObject *obj, QEvent *event);
  QPainter *painter;
  bool isDrawing;
};
