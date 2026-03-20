#include "Controller/Strategy/NormalStrategy.h"
#include "Interface/IViewController.h"
#include "Interface/IViewRenderer.h"
#include <cmath>
#include <QDebug>
#include <vtkImageData.h>
#include <vtkRenderer.h>
#include "Renderer/OverlayManager/CrosshairManager/SimpleCrosshairManager.h"
#include "Renderer/OverlayManager/IOverlayManager.h"

void NormalStrategy::HandleEvent(EventType type, int idx, const EventData& data) {
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

        m_dragging = true;
        m_window = m_controller->GetWindowWidth();
        m_level = m_controller->GetWindowLevel();

        this->LocatePoint(idx, pos);

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



void NormalStrategy::updateWindowLevel(int viewIndex) {
    m_controller->SetWindowLevel(m_window, m_level);
}

void NormalStrategy::LocatePoint(int viewIndex, int* pos) {
    auto viewer = m_controller->GetRenderer(viewIndex)->GetViewer();
	auto m_image = m_controller->GetImage();
    if (!viewer || !m_image) return;

    vtkRenderer* ren = viewer->GetRenderer();
    if (!ren) return;

    std::array<double, 3> worldPoint = m_controller->GetRenderer(viewIndex)->PickWorldPosition(pos[0], pos[1]); // 先更新拾取位置

    int dims[3];
    double spacing[3], origin[3];

    const_cast<vtkImageData*>(m_image)->GetDimensions(dims);
    const_cast<vtkImageData*>(m_image)->GetSpacing(spacing);
    const_cast<vtkImageData*>(m_image)->GetOrigin(origin);

    std::array<double, 3> worldMin, worldMax;
    for (int j = 0; j < 3; ++j) {
        worldMin[j] = origin[j];
        worldMax[j] = origin[j] + (dims[j] - 1) * spacing[j];
    }

    for (int i = 0; i < 3; ++i) {
        auto m_renderer = m_controller->GetRenderer(i);
        if (!m_renderer) continue;

        auto overlayManager = m_renderer->GetOverlayManager();
        if (overlayManager) {
            overlayManager->GetFeature<SimpleCrosshairManager>()->UpdateCrosshair(worldPoint,
                static_cast<ViewType>(i),
                worldMin.data(),
                worldMax.data());
        }
        m_renderer->SetCurrentClickWorldPos(worldPoint);
        m_renderer->RequestRender();
    }

    m_controller->UpdateSliceInternals(worldPoint);
}

void NormalStrategy::Clear(int viewIndex)
{
    auto renderer = m_controller->GetRenderer(viewIndex);
    if (!renderer) return;

    // 获取 overlay manager
    auto overlayMgr = renderer->GetOverlayManager();
    if (!overlayMgr) return;

    // 获取测距功能模块
    auto feature = overlayMgr->GetFeature<SimpleCrosshairManager>();
    if (!feature) return;

    feature->ClearAllMeasurement(); // 清除所有绘制
}