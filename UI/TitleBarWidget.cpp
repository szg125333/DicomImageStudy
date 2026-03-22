#include "TitleBarWidget.h"

TitleBarWidget::TitleBarWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	initUI();
	initConnections();
}

TitleBarWidget::~TitleBarWidget()
{}



void TitleBarWidget::initUI()
{
    QString buttonStyle = R"(
    QToolButton {
        background-color: #f0f0f0;
        border: 1px solid #d0d0d0;
        color: #333333;
        padding: 6px 12px;
        font-size: 10pt;
        border-radius: 4px;
        text-align: center;
    }

    QToolButton:hover {
        border: 1px solid #007ACC;
        background-color: #e6f0fa;
    }

    QToolButton:checked {
        background-color: #007ACC;
        color: white;
        border: 1px solid #005a9e;
    }

    QToolButton:pressed {
        background-color: #0062cc;
    }

    QToolButton:disabled {
        background-color: #f5f5f5;
        color: #aaaaaa;
        border: 1px solid #dddddd;
    }
)";

	this->setStyleSheet(buttonStyle);

    ui.OpenFolder->setStyleSheet(buttonStyle);
    ui.OpenFolder->setIcon(QIcon(":/DicomImageStudy/images/FlatStyle-Folder.png"));

    ui.NormalMode->setStyleSheet(buttonStyle);
    ui.NormalMode->setIcon(QIcon(":/DicomImageStudy/images/reset.png"));

    ui.DistanceMeasurement->setStyleSheet(buttonStyle);
    ui.DistanceMeasurement->setIcon(QIcon(":/DicomImageStudy/images/Ruler.png"));

    ui.AngleMeasurement->setStyleSheet(buttonStyle);
    ui.AngleMeasurement->setIcon(QIcon(":/DicomImageStudy/images/FlatStyle-Angle.png"));

    ui.ResetView->setStyleSheet(buttonStyle);
    ui.ResetView->setIcon(QIcon(":/DicomImageStudy/images/FlatStyle-Calibration.png"));

    ui.RoiMeasurement->setStyleSheet(buttonStyle);
    ui.RoiMeasurement->setIcon(QIcon(":/DicomImageStudy/images/FlatStyle-Frame.png"));

    ui.FreehandROI->setStyleSheet(buttonStyle);
    ui.FreehandROI->setIcon(QIcon(":/DicomImageStudy/images/FlatStyle-Lasso.png"));

    ui.toolButton_7->setStyleSheet(buttonStyle);
    ui.toolButton_7->setIcon(QIcon(":/DicomImageStudy/images/FlatStyle-Menu.png"));

    ui.toolButton_10->setStyleSheet(buttonStyle);
    ui.toolButton_10->setIcon(QIcon(":/DicomImageStudy/images/FlatStyle-Squares.png"));

    ui.ImageDrag->setStyleSheet(buttonStyle);
    ui.ImageDrag->setIcon(QIcon(":/DicomImageStudy/images/move.png"));

    ui.CrosshairRuler->setStyleSheet(buttonStyle);
    ui.CrosshairRuler->setIcon(QIcon(":/DicomImageStudy/images/crosshair1.png"));
}

void TitleBarWidget::initConnections()
{
	connect(ui.NormalMode, &QToolButton::toggled, this, &TitleBarWidget::on_NormalMode_toggled);
	connect(ui.DistanceMeasurement, &QToolButton::toggled, this, &TitleBarWidget::on_DistanceMeasurement_toggled);
	connect(ui.AngleMeasurement, &QToolButton::toggled, this, &TitleBarWidget::on_AngleMeasurement_toggled);
	connect(ui.RoiMeasurement, &QToolButton::toggled, this, &TitleBarWidget::on_RoiMeasurement_toggled);
	connect(ui.ResetView, &QToolButton::clicked, this, &TitleBarWidget::on_ResetView_clicked);
	connect(ui.FreehandROI, &QToolButton::toggled, this, &TitleBarWidget::on_FreehandROI_toggled);
	connect(ui.ImageDrag, &QToolButton::toggled, this, &TitleBarWidget::on_ToolButton_toggled);
	connect(ui.CrosshairRuler, &QToolButton::toggled, this, &TitleBarWidget::on_CrosshairRuler_toggled);
}


void TitleBarWidget::on_DistanceMeasurement_toggled(bool state)
{
    emit requestEnableDistanceMeasurement(InteractionMode::DistanceMeasure, state);
}

void TitleBarWidget::on_NormalMode_toggled(bool state)
{
    emit requestEnableNormalMode(InteractionMode::Normal, state);
}

void TitleBarWidget::on_AngleMeasurement_toggled(bool state)
{
	emit requestEnableAngleMeasurement(InteractionMode::AngleMeasure, state);
}

void TitleBarWidget::on_RoiMeasurement_toggled(bool state)
{
    emit requestRoiNormalMode(InteractionMode::RegistrationROI, state);
}

void TitleBarWidget::on_ResetView_clicked()
{
	emit requestResetViews();
}

void TitleBarWidget::on_FreehandROI_toggled(bool state)
{
    emit requestFreehandROIMode(InteractionMode::FreehandROI, state);

}

void TitleBarWidget::on_CrosshairRuler_toggled(bool state)
{
	emit requestCrosshairRulerMode(InteractionMode::CrosshairRuler, state);
}

void TitleBarWidget::on_ToolButton_toggled(bool state)
{
	emit requestMode(InteractionMode::ImageDrag, state);
}

