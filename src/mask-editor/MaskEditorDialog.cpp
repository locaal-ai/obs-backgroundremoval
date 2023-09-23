#include "MaskEditorDialog.hpp"

#include "plugin-support.h"

#include <obs-module.h>

/*
  * MaskEditorDialog

  * This dialog is used to edit the mask image for the Magic Eraser filter.
  * It is a simple dialog with a QLabel to show the mask image and two buttons
  * to save or cancel the changes.

  * The mask image is editable by the user. The user can draw on the mask image
  * using the mouse. The brush is a circle with a radius of 10
  * pixels.
*/

MaskEditorDialog::MaskEditorDialog(QWidget *parent, const cv::Mat &mask8UC1,
				   const cv::Mat &inputBGRA)
	: QDialog(parent), isDrawing(false)
{
	layout = new QVBoxLayout(this);
	setLayout(layout);
	// set the window title
	setWindowTitle("Mask Editor");

	// Add a widget to show the input image in the background
	QLabel *inputImage = new QLabel();
	layout->addWidget(inputImage);

	// Add a border around the input image
	inputImage->setFrameStyle(QFrame::Box | QFrame::Raised);

	// Convert the input image to a QImage
	QImage inputQImage = QImage(inputBGRA.data, inputBGRA.cols,
				    inputBGRA.rows, inputBGRA.step,
				    QImage::Format_RGBA8888);
	QImage maskQImage = QImage(mask8UC1.data, mask8UC1.cols, mask8UC1.rows,
				   mask8UC1.step, QImage::Format_Grayscale8);

  // convert mask image to RGBA
  maskQImage = maskQImage.convertToFormat(QImage::Format_RGBA8888);

  QImage alphaMask = maskQImage.createMaskFromColor(qRgb(0,0,0), Qt::MaskOutColor);

  // set the black pixels in the mask to transparent
  maskQImage.setAlphaChannel(alphaMask);

  QPainter p;
  p.begin(&inputQImage);
  p.setCompositionMode(QPainter::CompositionMode_SourceOver);

  p.drawImage(0, 0, alphaMask);

  p.end();


	// Display the input image
	inputImage->setPixmap(QPixmap::fromImage(inputQImage));

/*
	// Add a widget to show the mask image
	QLabel *maskImage = new QLabel();
	layout->addWidget(maskImage);

	// Add a border around the mask image
	maskImage->setFrameStyle(QFrame::Box | QFrame::Raised);

	// Allow the user to draw on the mask image
	maskImage->setMouseTracking(true);
	maskImage->setCursor(Qt::CrossCursor);

	obs_log(LOG_INFO,
		"MaskEditorDialog: mask8UC1.cols = %d, mask8UC1.rows = %d, mask8UC1.step = %d",
		mask8UC1.cols, mask8UC1.rows, mask8UC1.step);
	// Convert the mask image to a QImage

	// Display the mask image
	maskImage->setPixmap(QPixmap::fromImage(maskQImage));

	// Create a painter to draw on the mask image
	this->painter = new QPainter(&maskQImage);
	this->painter->setPen(QPen(Qt::white, 20, Qt::SolidLine, Qt::RoundCap,
				   Qt::RoundJoin));

	// Listen to mouse events on the mask image
	maskImage->installEventFilter(this);
*/
	// Save and Cancel buttons
	QHBoxLayout *buttonLayout = new QHBoxLayout();
	layout->addLayout(buttonLayout);

	QPushButton *saveButton = new QPushButton("Save");
	buttonLayout->addWidget(saveButton);

	QPushButton *cancelButton = new QPushButton("Cancel");
	buttonLayout->addWidget(cancelButton);

	// connect the buttons
	connect(saveButton, &QPushButton::clicked, this, &QDialog::accept);
	connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

	// set the dialog size
	resize(400, 400);

	// set the dialog to be modal
	setModal(true);
}

MaskEditorDialog::~MaskEditorDialog()
{
	delete layout;
	delete painter;
}

/*
  * eventFilter

  * This function is called when a mouse event occurs on the mask image.
  * It draws a white circle on the mask image at the mouse position.
*/

bool MaskEditorDialog::eventFilter(QObject *obj, QEvent *event)
{
	// check if the event is a mouse event
	if (event->type() == QEvent::MouseMove && this->isDrawing) {

		// cast the object to a QLabel
		QLabel *maskImage = static_cast<QLabel *>(obj);

		// get the mouse position
		QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
		QPoint mousePos = mouseEvent->pos();

		obs_log(LOG_INFO, "Mouse move event: x = %d, y = %d",
			mousePos.x(), mousePos.y());

		// draw a white circle on the mask image at the mouse position
		// this->painter->drawEllipse(mousePos, 10, 10);

		// update the mask image
		maskImage->setPixmap(
			QPixmap::fromImage(maskImage->pixmap().toImage()));

		// accept the event
		return true;
	}

	// If this is mouse press event, prime the painter
	if (event->type() == QEvent::MouseButtonPress) {
		// enable painting
		this->isDrawing = true;

		// accept the event
		return true;
	}

	// If this is mouse release event, stop painting
	if (event->type() == QEvent::MouseButtonRelease) {
		// disable painting
		this->isDrawing = false;

		// accept the event
		return true;
	}

	// pass the event to the base class
	return QDialog::eventFilter(obj, event);
}
