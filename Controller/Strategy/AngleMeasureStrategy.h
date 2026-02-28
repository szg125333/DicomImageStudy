#pragma once
#include "IInteractionStrategy.h"

class IViewController;

/// @brief 角度测量工具交互策略
/// 
/// 处理角度测量模式下的交互事件
class AngleMeasureStrategy : public IInteractionStrategy {
public:
    explicit AngleMeasureStrategy(IViewController* controller);
    ~AngleMeasureStrategy() override = default;

    void HandleEvent(EventType type, int viewIndex, const EventData& data) override;
};