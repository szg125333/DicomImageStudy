#pragma once

#include <QWidget>
#include <QMap>
#include <QString>
#include "ui_LeftToolWidget.h"

class QTableWidget;
class QLabel;

/**
 * @brief 左侧工具面板
 *
 * TabWidget 页签说明：
 *   Tab 1（tab）     → DICOM 元数据信息表
 *   Tab 2（tab_drag）→ 图像拖动物理位移实时记录面板（新增）
 */
class LeftToolWidget : public QWidget {
    Q_OBJECT

public:
    explicit LeftToolWidget(QWidget* parent = nullptr);
    ~LeftToolWidget() override;

    // ----------------------------------------------------------------
    //  Tab 1：DICOM 元数据
    // ----------------------------------------------------------------

    /// @brief 显示 DICOM 文件的元数据（Property → Value 表格）
    void SetDicomMetadata(const QMap<QString, QString>& metadata);

    // ----------------------------------------------------------------
    //  Tab 2：拖动位移面板（公开接口，供 MainWindow 连接信号）
    // ----------------------------------------------------------------

public slots:
    /**
     * @brief 更新拖动物理位移显示
     *
     * 由 MainWindow 连接到 ImageDragStrategy::dragUpdated 信号。
     *
     * @param viewIndex  事件来源视图（0=Axial 1=Sagittal 2=Coronal）
     * @param dx         X 轴累计位移（mm）
     * @param dy         Y 轴累计位移（mm）
     * @param dz         Z 轴累计位移（mm）
     * @param totalDist  三维总位移（mm）
     */
    void OnDragUpdated(int viewIndex, double dx, double dy, double dz, double totalDist);

    /**
     * @brief 清零拖动位移显示
     *
     * 由 MainWindow 连接到 ImageDragStrategy::dragReset 信号。
     */
    void OnDragReset();

private:
    void InitDicomInfoTab();    ///< 初始化 Tab 1
    void InitDragInfoTab();     ///< 初始化 Tab 2（新增）

    Ui::LeftToolWidgetClass* ui = nullptr;

    // ---- Tab 1 控件 ----
    QTableWidget* m_dicomTable = nullptr;

    // ---- Tab 2 控件（拖动信息）----
    /// 当前操作视图名称
    QLabel* m_labelView = nullptr;

    /// X / Y / Z 轴位移
    QLabel* m_labelDx = nullptr;
    QLabel* m_labelDy = nullptr;
    QLabel* m_labelDz = nullptr;

    /// 三维总位移
    QLabel* m_labelTotal = nullptr;

    /// 操作提示
    QLabel* m_labelHint = nullptr;
};
