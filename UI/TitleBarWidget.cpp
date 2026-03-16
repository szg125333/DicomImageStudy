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

    ui.DistanceMeasurement->setStyleSheet(buttonStyle);
    ui.DistanceMeasurement->setIcon(QIcon(":/DicomImageStudy/images/Ruler.png"));

    ui.AngleMeasurement->setStyleSheet(buttonStyle);
    ui.AngleMeasurement->setIcon(QIcon(":/DicomImageStudy/images/FlatStyle-Angle.png"));

    ui.OpenFolder->setStyleSheet(buttonStyle);
    ui.OpenFolder->setIcon(QIcon(":/DicomImageStudy/images/FlatStyle-Folder.png"));
}

void TitleBarWidget::initConnections()
{
	connect(ui.NormalMode, &QToolButton::toggled, this, &TitleBarWidget::on_NormalMode_toggled);
	connect(ui.DistanceMeasurement, &QToolButton::toggled, this, &TitleBarWidget::on_DistanceMeasurement_toggled);
	connect(ui.AngleMeasurement, &QToolButton::toggled, this, &TitleBarWidget::on_AngleMeasurement_toggled);
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

