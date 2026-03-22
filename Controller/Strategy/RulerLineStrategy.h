#pragma once

#include "Controller/Strategy/AbstractMeasureStrategy.h"
#include "Renderer/OverlayManager/RulerLineManager/SimpleRulerLineManager.h"

/**
 * @brief 双线独立刻度尺交互策略
 *
 * 模式进入时：在当前视图图像中心初始化两条线
 *
 * 操作：
 *   左键命中水平线 + 拖动 → 水平线上下移动
 *   左键命中垂直线 + 拖动 → 垂直线左右移动
 *   左键未命中         → 忽略
 *   滚轮               → 由 Controller 固定路由到 NormalStrategy（切片）
 *
 * 拖动算法：屏幕像素增量 × ParallelScale 换算，无抖动。
 */
class RulerLineStrategy : public AbstractMeasureStrategy {
public:
    explicit RulerLineStrategy(IViewController* controller);

    void HandleEvent(EventType type, int viewIndex, const EventData& data) override;
    void Clear(int viewIndex) override;
    void OnActivated() override;

private:
    bool m_isDragging = false;
    int  m_activeViewIndex = -1;

    SimpleRulerLineManager::HitLine m_dragTarget =
        SimpleRulerLineManager::HitLine::None;

    int m_lastScreenX = 0;
    int m_lastScreenY = 0;

    bool m_placed[3] = { false, false, false };
};
