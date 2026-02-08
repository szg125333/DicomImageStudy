#pragma once
#include "IInteractionStrategy.h"
#include <QDebug>

class ZoomStrategy : public IInteractionStrategy {
public:
    ZoomStrategy(ThreeViewController* ctrl) : m_ctrl(ctrl) {}
    void HandleEvent(EventType type, int idx, void* data) override {
        if (type == EventType::WheelForward) {
            qDebug() << "Zoom in on view" << idx;
        }
        else if (type == EventType::WheelBackward) {
            qDebug() << "Zoom out on view" << idx;
        }
    }
private:
    ThreeViewController* m_ctrl;
};
