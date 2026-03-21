// ================================================================
//  RegistrationROIStrategy.h
// ================================================================
#pragma once
#include "IInteractionStrategy.h"

/**
 * @brief 配准 ROI 选择策略（待实现）
 *
 * 用于在配准流程中选取感兴趣区域（ROI）。
 * 当前为占位实现，后续按配准模块需求补全。
 */
class RegistrationROIStrategy : public IInteractionStrategy {
public:
    explicit RegistrationROIStrategy(IViewController* controller)
        : IInteractionStrategy(controller) {
    }

    void HandleEvent(EventType type, int viewIndex, const EventData& data) override;
    void Clear(int viewIndex) override;
};
