#pragma once
#include "IInteractionStrategy.h"

class IViewController;

/// @brief 配准 ROI（盒子）模式交互策略
/// 
/// 处理配准 ROI 模式下的交互事件
class RegistrationROIStrategy : public IInteractionStrategy {
public:
    explicit RegistrationROIStrategy(IViewController* controller);
    ~RegistrationROIStrategy() override = default;

    void HandleEvent(EventType type, int viewIndex, void* data) override;

private:
    IViewController* m_controller = nullptr;
};