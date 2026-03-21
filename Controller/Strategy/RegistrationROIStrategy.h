#pragma once

#include "AbstractMeasureStrategy.h"
#include "Common/ViewTypes.h"

/**
 * @brief ROI 矩形框绘制交互策略
 *
 * 操作流程：
 *   1. 左键按下          → 记录起始角点，开始绘制
 *   2. 左键拖拽（移动）   → 实时更新预览矩形框
 *   3. 左键释放          → 完成 ROI，计算并显示统计信息
 *   4. 右键 / Esc 键     → 取消当前未完成的 ROI
 *   5. Delete 键         → 删除最后一个已完成的 ROI
 *   6. Ctrl + Delete     → 清除所有 ROI
 *
 * ROI 统计项（在矩形框右上角以悬浮标签显示）：
 *   - 均值（Mean）
 *   - 标准差（Std Dev）
 *   - 最小值（Min）
 *   - 最大值（Max）
 *   - 面积（mm²）
 *   - 像素数（Pixels）
 */
class RegistrationROIStrategy : public AbstractMeasureStrategy {
public:
    explicit RegistrationROIStrategy(IViewController* controller);

    void HandleEvent(EventType type, int viewIndex, const EventData& data) override;
    void Clear(int viewIndex) override;

private:
    /// 当前正在绘制的视图索引（-1 = 未在绘制）
    int      m_drawingViewIndex = -1;

    /// 当前绘制所在的视图方向
    ViewType m_drawingViewType = ViewType::None;

    /// 当前绘制所在的切片索引
    int      m_drawingSlice = 0;

    /// 是否正在拖拽（鼠标按下且未释放）
    bool     m_isDragging = false;
};