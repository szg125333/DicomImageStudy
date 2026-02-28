#pragma once
#include "IInteractionStrategy.h"
#include <QDebug>

class IViewController;

class NormalStrategy : public IInteractionStrategy {
public:
    explicit NormalStrategy(IViewController* ctrl)
        : IInteractionStrategy(ctrl)
    {
    }
    void HandleEvent(EventType type, int idx, const EventData& data) override;

private:

    int m_lastPos[2] = { 0,0 };
    bool m_dragging = false;
    double m_window = 400;   // 初始窗宽
    double m_level = 40;    // 初始窗位
    double m_sensitivityX = 2.0; // 水平灵敏度
    double m_sensitivityY = 2.0; // 垂直灵敏度

    void updateWindowLevel(int viewIndex);
};
