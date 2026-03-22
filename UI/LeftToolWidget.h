#pragma once

#include <QWidget>
#include <QMap>
#include <QString>
#include "ui_LeftToolWidget.h"

class QTableWidget;

/**
 * @brief 左侧工具面板
 *
 * 所有控件均在 LeftToolWidget.ui 中设计，.cpp 只负责逻辑。
 *
 * Tab 1（tab）     — DICOM 元数据信息表
 * Tab 2（tab_2）   — 图像拖动物理位移实时显示
 */
class LeftToolWidget : public QWidget {
    Q_OBJECT

public:
    explicit LeftToolWidget(QWidget* parent = nullptr);
    ~LeftToolWidget() override;

    // ----------------------------------------------------------------
    //  Tab 1 接口
    // ----------------------------------------------------------------

    /// @brief 显示 DICOM 元数据（Property → Value 两列表格）
    void SetDicomMetadata(const QMap<QString, QString>& metadata);

public slots:
    // ----------------------------------------------------------------
    //  Tab 2 接口（由 MainWindow 连接到 Controller 信号）
    // ----------------------------------------------------------------

    /**
     * @brief 更新拖动位移显示
     * @param viewIndex  视图索引（0=Axial 1=Sagittal 2=Coronal）
     * @param dx         X 轴累计位移（mm）
     * @param dy         Y 轴累计位移（mm）
     * @param dz         Z 轴累计位移（mm）
     * @param totalDist  三维总位移（mm）
     */
    void OnDragUpdated(int viewIndex,
        double dx, double dy, double dz,
        double totalDist);

    /// @brief 清零拖动位移显示（右键单击或切换模式时触发）
    void OnDragReset();

private:
    void InitDicomInfoTab();   ///< 初始化 Tab 1（动态创建 QTableWidget）

    Ui::LeftToolWidgetClass* ui = nullptr;

    /// DICOM 元数据表格（Tab 1 动态创建，其余控件均在 .ui 中）
    QTableWidget* m_dicomTable = nullptr;
};
