#pragma once
#include "IInteractionStrategy.h"
#include <array>

class IViewController;

class DistanceMeasureStrategy : public IInteractionStrategy {
public:
    explicit DistanceMeasureStrategy(IViewController* controller);
    void HandleEvent(EventType type, int viewIndex, void* data) override;

private:
    IViewController* m_controller = nullptr;

    // 存储起始点的世界坐标（不是屏幕坐标！）
    bool m_hasFirstPoint = false;
    std::array<double, 3> m_startWorldPos;
    int m_startViewIndex = -1;
};