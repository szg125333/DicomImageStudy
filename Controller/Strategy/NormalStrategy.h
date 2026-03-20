#pragma once
#include "IInteractionStrategy.h"
#include <QDebug>
#include <array>

class IViewController;

class NormalStrategy : public IInteractionStrategy {
public:
    explicit NormalStrategy(IViewController* m_controller)
        : IInteractionStrategy(m_controller)
    {
    }
    void HandleEvent(EventType type, int idx, const EventData& data) override;
    void Clear(int viewIndex) override;

private:

    int m_lastPos[2] = { 0,0 };
    //int m_fristClickedPos[2] = { 0,0 };
    std::array<double, 3> m_initialFocalPoint;
    bool m_dragging = false;
    double m_window = 400;   // 初始窗宽
    double m_level = 40;    // 初始窗位
    double m_sensitivityX = 2.0; // 水平灵敏度
    double m_sensitivityY = 2.0; // 垂直灵敏度

    void updateWindowLevel(int viewIndex);
};
