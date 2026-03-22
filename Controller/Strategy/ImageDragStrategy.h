#pragma once

#include "IInteractionStrategy.h"
#include <array>
#include <functional>

/**
 * @brief 图像平移拖动策略
 *
 * 激活后左键按住拖动即可平移视图中的图像（移动相机焦点）。
 * 使用 std::function 回调代替 Qt 信号，避免继承 QObject 与
 * unique_ptr<IInteractionStrategy> 的生命周期冲突。
 *
 * 操作：
 *   左键按下 + 拖动 → 平移图像，实时上报累计物理位移
 *   右键单击        → 累计位移清零
 *   切换模式        → Clear() 自动清零
 *
 * 集成：
 *   创建策略后调用 SetDragCallback / SetResetCallback 注入回调，
 *   由 ThreeViewController 转发为 Qt 信号供 LeftToolWidget 使用。
 */
class ImageDragStrategy : public IInteractionStrategy {
public:
    /// 拖动帧回调：(viewIndex, totalDx, totalDy, totalDz, totalDist_mm)
    using DragCallback = std::function<void(int, double, double, double, double)>;
    /// 清零回调
    using ResetCallback = std::function<void()>;

    explicit ImageDragStrategy(IViewController* controller);

    void HandleEvent(EventType type, int viewIndex, const EventData& data) override;
    void Clear(int viewIndex) override;

    /// @brief 注册拖动更新回调
    void SetDragCallback(DragCallback cb) { m_dragCb = std::move(cb); }

    /// @brief 注册清零回调
    void SetResetCallback(ResetCallback cb) { m_resetCb = std::move(cb); }

private:
    void ResetDisplacement();

    bool m_isDragging = false;
    int  m_activeViewIndex = -1;

    std::array<double, 3> m_lastWorldPos = { 0.0, 0.0, 0.0 };

    double m_totalDx = 0.0;
    double m_totalDy = 0.0;
    double m_totalDz = 0.0;

    DragCallback  m_dragCb;
    ResetCallback m_resetCb;
};
