#pragma once
#include "IInteractionStrategy.h"

class IViewController;

/// @brief 手工不规则轮廓交互策略
/// 
/// 处理手工不规则轮廓绘制模式下的交互事件
class HandIrregularContourStrategy : public IInteractionStrategy {
public:
    explicit HandIrregularContourStrategy(IViewController* controller);
    ~HandIrregularContourStrategy() override = default;

    void HandleEvent(EventType type, int viewIndex, void* data) override;

private:
    IViewController* m_controller = nullptr;
};