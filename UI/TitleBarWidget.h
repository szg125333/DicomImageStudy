#pragma once

#include <QWidget>
#include "ui_TitleBarWidget.h"
#include "Common/InteractionMode.h"

class TitleBarWidget : public QWidget
{
	Q_OBJECT

public:
	TitleBarWidget(QWidget *parent = nullptr);
	~TitleBarWidget();

signals:
	void requestOpenFolder();
	void requestEnableDistanceMeasurement(InteractionMode mode,bool state);
	void requestEnableAngleMeasurement(InteractionMode mode, bool state);
	void requestEnableNormalMode(InteractionMode mode, bool state);
	void requestRoiNormalMode(InteractionMode mode, bool state);
	void requestResetViews();
	void requestFreehandROIMode(InteractionMode mode, bool state);

	void requestMode(InteractionMode mode, bool state);

private slots:
	void on_NormalMode_toggled(bool state);
	void on_DistanceMeasurement_toggled(bool state);
	void on_AngleMeasurement_toggled(bool state);
	void on_RoiMeasurement_toggled(bool state);
	void on_ResetView_clicked();
	void on_FreehandROI_toggled(bool state);

	void on_ToolButton_toggled(bool state);

private:
	void initUI();          // ← 新增：初始化 UI 外观
	void initConnections(); // ← 新增：初始化信号槽

	Ui::TitleBarWidgetClass ui;
};

