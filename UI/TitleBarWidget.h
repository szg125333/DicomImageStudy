#pragma once

#include <QWidget>
#include <functional>
#include <vector>
#include "ui_TitleBarWidget.h"
#include "Common/InteractionMode.h"
#include "Common/ROIDisplayInfo.h"

class ContourListPopup;

class TitleBarWidget : public QWidget
{
    Q_OBJECT

public:
    TitleBarWidget(QWidget* parent = nullptr);
    ~TitleBarWidget();

    /// @brief 注入轮廓列表数据提供者（解耦，不依赖具体实现）
    void SetContourListProvider(std::function<std::vector<ROIDisplayInfo>()> provider);

signals:
    /// @brief 统一的模式切换信号（所有模式按钮共用）
    void requestMode(InteractionMode mode, bool state);

    /// @brief 重置视图（独立功能，不是模式切换）
    void requestResetViews();

    /// @brief ROI 可见性变化
    void roiVisibilityChanged(int roiNumber, bool visible);

private:
    void initUI();
    void initConnections();

    /// @brief 批量绑定工具：按钮 toggled → 发 requestMode 信号
    void bindModeButton(QToolButton* button, InteractionMode mode);

    Ui::TitleBarWidgetClass ui;
    ContourListPopup* m_contourPopup = nullptr;
    std::function<std::vector<ROIDisplayInfo>()> m_contourListProvider;
};