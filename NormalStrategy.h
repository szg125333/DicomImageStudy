#pragma once
#include "IInteractionStrategy.h"
#include <QDebug>

class ThreeViewController;

class NormalStrategy : public IInteractionStrategy {
public:
    NormalStrategy(ThreeViewController* ctrl) : m_ctrl(ctrl) {}

    void HandleEvent(EventType type, int idx, void* data) override;

private:
    ThreeViewController* m_ctrl;

    // 用于窗宽窗位调整
    int m_lastPos[2] = { 0,0 };
    bool m_dragging = false;
    double m_window = 400;   // 初始窗宽
    double m_level = 40;    // 初始窗位
    double m_sensitivityX = 1.0; // 水平灵敏度
    double m_sensitivityY = 1.0; // 垂直灵敏度

    void updateWindowLevel(int viewIndex);
};
