#pragma once
#include "IInteractionStrategy.h"

class IViewController;

class DistanceMeasureStrategy : public IInteractionStrategy {
public:
    explicit DistanceMeasureStrategy(IViewController* controller);
    void HandleEvent(EventType type, int viewIndex, void* data) override;

private:
    IViewController* m_controller = nullptr;
    bool m_hasFirstPoint = false;
    int m_firstPoint[2] = { 0, 0 };
    int m_firstViewIndex = -1;
};