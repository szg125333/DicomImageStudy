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
	void requestEnableDistanceMeasurement();

private slots:
	void on_DistanceMeasurement_clicked();

private:
	Ui::TitleBarWidgetClass ui;
};

