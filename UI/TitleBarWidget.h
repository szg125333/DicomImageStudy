#pragma once

#include <QWidget>
#include "ui_TitleBarWidget.h"
#include "Common/InteractionMode.h"
#include "ContourListPopup.h"

class TitleBarWidget : public QWidget
{
	Q_OBJECT

public:
	TitleBarWidget(QWidget *parent = nullptr);
	~TitleBarWidget();
	void UpdateContourList(const std::vector<ROIDisplayInfo>& roiList);

signals:
	void requestOpenFolder();
	void requestEnableDistanceMeasurement(InteractionMode mode,bool state);
	void requestEnableAngleMeasurement(InteractionMode mode, bool state);
	void requestEnableNormalMode(InteractionMode mode, bool state);
	void requestRoiNormalMode(InteractionMode mode, bool state);
	void requestResetViews();
	void requestFreehandROIMode(InteractionMode mode, bool state);
	void requestCrosshairRulerMode(InteractionMode mode, bool state);

	void requestMode(InteractionMode mode, bool state);

	void roiVisibilityChanged(int roiNumber, bool visible);
	void requestContourList();  // 请求 Controller 提供 ROI 列表

private slots:
	void on_NormalMode_toggled(bool state);
	void on_DistanceMeasurement_toggled(bool state);
	void on_AngleMeasurement_toggled(bool state);
	void on_RoiMeasurement_toggled(bool state);
	void on_ResetView_clicked();
	void on_FreehandROI_toggled(bool state);
	void on_CrosshairRuler_toggled(bool state);

	void on_ToolButton_toggled(bool state);

private:
	void initUI();          // ← 新增：初始化 UI 外观
	void initConnections(); // ← 新增：初始化信号槽

	Ui::TitleBarWidgetClass ui;

	ContourListPopup* m_contourPopup = nullptr;

};

