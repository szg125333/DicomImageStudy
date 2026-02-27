#pragma once
#include "IInteractionStrategy.h"

class IViewController;

/// @brief 手动平移/旋转（移动对象）交互策略
/// 
/// 处理手动平移和旋转操作
class ManualMoveStrategy : public IInteractionStrategy {
public:
    explicit ManualMoveStrategy(IViewController* controller);
    ~ManualMoveStrategy() override = default;

    void HandleEvent(EventType type, int viewIndex, const EventData& data) override;

private:
    IViewController* m_controller = nullptr;
};