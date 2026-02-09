#pragma once
#include "IInteractionStrategy.h"
#include "ThreeViewController.h"
#include "ViewTypes.h"

class SliceStrategy : public IInteractionStrategy {
public:
    SliceStrategy(ThreeViewController* ctrl) : m_ctrl(ctrl) {}
    void HandleEvent(EventType type, int idx, void* data) override {
        if (type == EventType::WheelForward) {
            m_ctrl->RequestSetSlice(static_cast<ViewType>(idx),
                m_ctrl->GetSlice(static_cast<ViewType>(idx)) + 1);
        }
        else if (type == EventType::WheelBackward) {
            m_ctrl->RequestSetSlice(static_cast<ViewType>(idx),
                m_ctrl->GetSlice(static_cast<ViewType>(idx)) - 1);
        }
    }
private:
    ThreeViewController* m_ctrl;
};
