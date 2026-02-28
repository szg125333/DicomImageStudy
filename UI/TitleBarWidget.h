#pragma once

#include <QWidget>
#include "ui_TitleBarWidget.h"

class TitleBarWidget : public QWidget
{
	Q_OBJECT

public:
	TitleBarWidget(QWidget *parent = nullptr);
	~TitleBarWidget();

signals:
	void requestEnableDistanceMeasurement(bool state);

private slots:
	void on_DistanceMeasurement_toggled(bool state);

private:
	Ui::TitleBarWidgetClass ui;
};

