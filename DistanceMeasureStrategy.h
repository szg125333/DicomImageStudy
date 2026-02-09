#pragma once
#include "IInteractionStrategy.h"

class IViewController;

/// @brief 距离测量工具交互策略
/// 
/// 处理距离测量模式下的交互事件
class DistanceMeasureStrategy : public IInteractionStrategy {
public:
    explicit DistanceMeasureStrategy(IViewController* controller);
    ~DistanceMeasureStrategy() override = default;

    void HandleEvent(EventType type, int viewIndex, void* data) override;

private:
    IViewController* m_controller = nullptr;
};