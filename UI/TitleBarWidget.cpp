#include "TitleBarWidget.h"

TitleBarWidget::TitleBarWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.DistanceMeasurement, &QToolButton::toggled, this, &TitleBarWidget::on_DistanceMeasurement_clicked);
}

TitleBarWidget::~TitleBarWidget()
{}


void TitleBarWidget::on_DistanceMeasurement_clicked()
{
	emit requestEnableDistanceMeasurement();
}

