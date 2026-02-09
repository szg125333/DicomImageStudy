#include "NormalStrategy.h"
#include "ThreeViewController.h"
#include "IViewRenderer.h"
#include <cmath>
#include <QDebug>  // 添加调试

void NormalStrategy::HandleEvent(EventType type, int idx, void* data) {
    auto pos = static_cast<int*>(data);
    if (!pos) return;

    if (type == EventType::WheelForward) {
        m_ctrl->ChangeSlice(idx, +1);
    }
    else if (type == EventType::WheelBackward) {
        m_ctrl->ChangeSlice(idx, -1);
    }
    else if (type == EventType::LeftPress) {
        // 第一次按下
        m_lastPos[0] = pos[0];
        m_lastPos[1] = pos[1];
        m_dragging = true;
        m_window = m_ctrl->GetWindowWidth();
        m_level = m_ctrl->GetWindowLevel();

        qDebug() << "LeftPress:" << pos[0] << pos[1]
            << "WW=" << m_window << "WL=" << m_level;
    }
    else if (type == EventType::LeftMove) {
        qDebug() << "LeftMove event received at" << pos[0] << pos[1];

        if (!m_dragging) {
            qDebug() << "  -> m_dragging is false, ignoring";
            return;
        }

        int dx = pos[0] - m_lastPos[0];
        int dy = pos[1] - m_lastPos[1];

        qDebug() << "  -> dx=" << dx << "dy=" << dy;

        // ✅ 改进：即使 dx/dy 为 0 也要处理
        // 移除阈值检查，只要有移动就更新
        if (dx != 0 || dy != 0) {
            m_lastPos[0] = pos[0];
            m_lastPos[1] = pos[1];

            // ✅ 计算变化量
            double dww = dx * m_sensitivityX;
            double dwl = -dy * m_sensitivityY;  // Y 轴通常反向

            m_window += dww;
            m_level += dwl;

            // 范围限制
            if (m_window < 1) m_window = 1;
            if (m_window > 4095) m_window = 4095;  // 根据需要调整

            // m_level 可以是负数，看你的需求

            qDebug() << "  -> Updated WW=" << m_window << "WL=" << m_level;

            updateWindowLevel(idx);  // 实时更新
        }
    }
    else if (type == EventType::LeftRelease) {
        qDebug() << "LeftRelease at" << pos[0] << pos[1];

        m_dragging = false;

        // 检查是否真的是单击（没有移动）
        int dx = pos[0] - m_lastPos[0];
        int dy = pos[1] - m_lastPos[1];
        int clickThreshold = 2;

        qDebug() << "  -> Final dx=" << dx << "dy=" << dy;

        if (std::abs(dx) < clickThreshold && std::abs(dy) < clickThreshold) {
            qDebug() << "  -> This is a click, locating point";
            m_ctrl->LocatePoint(idx, pos);
        }
    }
}

void NormalStrategy::updateWindowLevel(int viewIndex) {
    m_ctrl->SetWindowLevel(m_window, m_level);
}