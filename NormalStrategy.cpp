#include "NormalStrategy.h"
#include "ThreeViewController.h"
#include "IViewRenderer.h"
#include <vtkImageViewer2.h>
#include <vtkRenderer.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>

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
        // 记录初始位置
        m_lastPos[0] = pos[0];
        m_lastPos[1] = pos[1];
        m_dragging = true;
    }
    else if (type == EventType::LeftMove && m_dragging) {
        // 拖动调整窗宽窗位
        int dx = pos[0] - m_lastPos[0];
        int dy = pos[1] - m_lastPos[1];
        m_lastPos[0] = pos[0];
        m_lastPos[1] = pos[1];

        m_window = m_ctrl->GetWindowWidth();
        m_level = m_ctrl->GetWindowLevel();

        m_window += dx * m_sensitivityX;
        m_level += dy * m_sensitivityY;

        updateWindowLevel(idx);
    }
    else if (type == EventType::LeftRelease) {
        // 单击定位（如果没有发生拖动）
        //if (!m_dragging) {
            m_ctrl->LocatePoint(idx, pos);
        //}
        m_dragging = false;
    }
}

void NormalStrategy::updateWindowLevel(int viewIndex) {
    m_ctrl->SetWindowLevel(m_window, m_level);
}
