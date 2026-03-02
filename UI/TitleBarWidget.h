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
	void initUI();          // ← 新增：初始化 UI 外观
	void initConnections(); // ← 新增：初始化信号槽

	Ui::TitleBarWidgetClass ui;
};

