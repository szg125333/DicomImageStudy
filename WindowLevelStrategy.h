#pragma once
#include "IInteractionStrategy.h"
#include <QDebug>

class ThreeViewController;

class WindowLevelStrategy : public IInteractionStrategy {
public:
    explicit WindowLevelStrategy(ThreeViewController* ctrl);

    void HandleEvent(EventType type, int viewIndex, void* data) override;

private:
    ThreeViewController* m_ctrl;
    int m_lastPos[2] = { 0,0 };
    double m_window = 400;   // 初始窗宽
    double m_level = 40;    // 初始窗位
    double m_sensitivityX = 1.0; // 鼠标水平灵敏度
    double m_sensitivityY = 1.0; // 鼠标垂直灵敏度

    void updateWindowLevel(int viewIndex);
};
