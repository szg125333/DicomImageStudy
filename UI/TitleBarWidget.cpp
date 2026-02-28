#include "TitleBarWidget.h"

TitleBarWidget::TitleBarWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.DistanceMeasurement, &QToolButton::toggled, this, &TitleBarWidget::on_DistanceMeasurement_toggled);
}

TitleBarWidget::~TitleBarWidget()
{}


void TitleBarWidget::on_DistanceMeasurement_toggled(bool state)
{
	emit requestEnableDistanceMeasurement(state);
}

