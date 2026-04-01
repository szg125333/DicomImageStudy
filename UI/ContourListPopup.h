#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QScrollArea>
#include <vector>
#include <map>
#include "Common/ROIDisplayInfo.h"

/// @brief 轮廓列表浮动面板
///
/// 点击 ContourOverlay 按钮时弹出，显示所有 ROI 的列表，
/// 每个 ROI 一行：彩色色块 + 名称 + checkbox。
/// 勾选/取消 checkbox 时发出信号通知外部。
class ContourListPopup : public QWidget {
    Q_OBJECT

public:
    explicit ContourListPopup(QWidget* parent = nullptr);
    ~ContourListPopup() override = default;

    /// @brief 更新 ROI 列表
    void SetROIList(const std::vector<ROIDisplayInfo>& roiList);

    /// @brief 清空列表
    void Clear();

signals:
    /// @brief ROI 可见性变化
    void roiVisibilityChanged(int roiNumber, bool visible);

    /// @brief 全部显示/隐藏
    void allVisibilityChanged(bool visible);

protected:
    /// @brief 点击面板外部时自动隐藏
    void changeEvent(QEvent* event) override;

private:
    void buildUI();
    QWidget* createROIItem(const ROIDisplayInfo& roi);

    QVBoxLayout* m_mainLayout = nullptr;
    QWidget* m_listContainer = nullptr;
    QVBoxLayout* m_listLayout = nullptr;
    QScrollArea* m_scrollArea = nullptr;

    std::map<int, QCheckBox*> m_checkboxes;  // roiNumber → checkbox
};