#include "Controller/Strategy/NormalStrategy.h"
#include "Interface/IViewController.h"
#include "Interface/IViewRenderer.h"
#include <cmath>
#include <QDebug>

void NormalStrategy::HandleEvent(EventType type, int idx, const EventData& data) {
    //auto pos = static_cast<int*>(data);
    int pos[2];
	pos[0] = data.mousePosX;
	pos[1] = data.mousePosY;

    if (!pos) return;

    if (type == EventType::WheelForward) {
        m_controller->ChangeSlice(idx, +1);
    }
    else if (type == EventType::WheelBackward) {
        m_controller->ChangeSlice(idx, -1);
    }
    else if (type == EventType::LeftPress) {
        // 第一次按下
        m_lastPos[0] = pos[0];
        m_lastPos[1] = pos[1];

        m_initialFocalPoint = m_controller->GetRenderer(idx)->PickWorldPosition(pos[0], pos[1]);

        //m_fristClickedPos[0] = pos[0];
        //m_fristClickedPos[1] = pos[1];

        m_dragging = true;
        m_window = m_controller->GetWindowWidth();
        m_level = m_controller->GetWindowLevel();

        m_controller->LocatePoint(idx, pos);
    }
    else if (type == EventType::LeftMove) {
        if (!m_dragging) {
            return;
        }

        int dx = pos[0] - m_lastPos[0];
        int dy = pos[1] - m_lastPos[1];
        if (dx == 0 && dy == 0) return;

        m_lastPos[0] = pos[0];
        m_lastPos[1] = pos[1];

        if (data.ctrlPressed) {
            double zoomFactor = 1.0 + dy * 0.01; // 向上拖动放大，向下缩小
            m_controller->Zoom(idx, zoomFactor, m_initialFocalPoint); // 需要控制器支持此接口
        }
        else {
            // === 默认：调整窗宽窗位 ===
            double dww = dx * m_sensitivityX;
            double dwl = -dy * m_sensitivityY;
            m_window += dww;
            m_level += dwl;
            if (m_window < 1) m_window = 1;
            if (m_window > 4095) m_window = 4095;
            qDebug() << "  -> Updated WW=" << m_window << "WL=" << m_level;
            updateWindowLevel(idx);
        }
    }
    else if (type == EventType::LeftRelease) {
        qDebug() << "LeftRelease at" << pos[0] << pos[1];

        m_dragging = false;

        int dx = pos[0] - m_lastPos[0];
        int dy = pos[1] - m_lastPos[1];
        int clickThreshold = 2;
    }
}

void NormalStrategy::Clear(int viewIndex)
{
}

void NormalStrategy::updateWindowLevel(int viewIndex) {
    m_controller->SetWindowLevel(m_window, m_level);
}