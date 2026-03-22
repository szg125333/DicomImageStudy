#include "RulerLineStrategy.h"
#include "Interface/IViewController.h"
#include "Interface/IViewRenderer.h"
#include "Renderer/OverlayManager/IOverlayManager.h"
#include "Renderer/OverlayManager/RulerLineManager/SimpleRulerLineManager.h"
#include "Utils/ViewportUtils.h"

#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkImageViewer2.h>
#include <vtkRenderWindow.h>

RulerLineStrategy::RulerLineStrategy(IViewController* controller)
    : AbstractMeasureStrategy(controller)
{
}

void RulerLineStrategy::HandleEvent(EventType type, int viewIndex, const EventData& data)
{
    if (!m_controller) return;
    if (m_activeViewIndex != -1 && m_activeViewIndex != viewIndex) return;

    auto* renderer = m_controller->GetRenderer(viewIndex);
    if (!renderer) return;

    auto* overlayMgr = renderer->GetOverlayManager();
    if (!overlayMgr) return;

    auto* rulerMgr = overlayMgr->GetFeature<SimpleRulerLineManager>();
    if (!rulerMgr) return;

    // 首次进入：放置在图像中心
    if (!m_placed[viewIndex]) {
        auto viewer = renderer->GetViewer();
        if (viewer) {
            rulerMgr->PlaceAtImageCenter(viewer.Get(),
                renderer->GetCurrentViewType(),
                renderer->GetSlice());
            rulerMgr->SetVisible(true);
            m_placed[viewIndex] = true;
            renderer->RequestRender();
        }
    }

    switch (type) {

    case EventType::LeftPress: {
        auto hit = rulerMgr->HitTest(data.mousePosX, data.mousePosY);
        if (hit != SimpleRulerLineManager::HitLine::None) {
            m_isDragging = true;
            m_activeViewIndex = viewIndex;
            m_dragTarget = hit;
            m_lastScreenX = data.mousePosX;
            m_lastScreenY = data.mousePosY;
        }
        break;
    }

    case EventType::LeftMove: {
        if (!m_isDragging) break;

        const int pixDx = data.mousePosX - m_lastScreenX;
        const int pixDy = data.mousePosY - m_lastScreenY;
        if (pixDx == 0 && pixDy == 0) break;

        m_lastScreenX = data.mousePosX;
        m_lastScreenY = data.mousePosY;

        auto viewer = renderer->GetViewer();
        if (!viewer || !viewer->GetRenderer()) break;

        auto* camera = viewer->GetRenderer()->GetActiveCamera();
        if (!camera || !camera->GetParallelProjection()) break;

        const int* ws = viewer->GetRenderWindow()->GetSize();
        if (ws[0] <= 0 || ws[1] <= 0) break;

        // 使用统一工具函数（OpenGL Y，不取反）
        auto delta = ViewportUtils::PixelDeltaToWorld(pixDx, pixDy, camera, ws[0], ws[1]);

        if (m_dragTarget == SimpleRulerLineManager::HitLine::Horizontal)
            rulerMgr->MoveHorizontalLine(delta);
        else if (m_dragTarget == SimpleRulerLineManager::HitLine::Vertical)
            rulerMgr->MoveVerticalLine(delta);

        renderer->RequestRender();
        break;
    }

    case EventType::LeftRelease: {
        m_isDragging = false;
        m_activeViewIndex = -1;
        m_dragTarget = SimpleRulerLineManager::HitLine::None;
        break;
    }

    default: break;
    }
}

void RulerLineStrategy::Clear(int viewIndex)
{
    if (!m_controller) return;
    auto* renderer = m_controller->GetRenderer(viewIndex);
    if (!renderer) return;
    auto* mgr = renderer->GetOverlayManager();
    if (!mgr) return;
    auto* ruler = mgr->GetFeature<SimpleRulerLineManager>();
    if (ruler) ruler->SetVisible(false);

    m_isDragging = false;
    m_activeViewIndex = -1;
    m_dragTarget = SimpleRulerLineManager::HitLine::None;
    m_placed[viewIndex] = false;
}

void RulerLineStrategy::OnActivated()
{
    if (!m_controller) return;

    // 遍历三个视图，逐一放置刻度线
    for (int i = 0; i < 3; ++i) {
        auto* renderer = m_controller->GetRenderer(i);
        if (!renderer) continue;

        auto* overlayMgr = renderer->GetOverlayManager();
        if (!overlayMgr) continue;

        auto* rulerMgr = overlayMgr->GetFeature<SimpleRulerLineManager>();
        if (!rulerMgr) continue;

        auto viewer = renderer->GetViewer();
        if (!viewer) continue;

        // 在图像中心放置，立即可见
        rulerMgr->PlaceAtImageCenter(
            viewer.Get(),
            renderer->GetCurrentViewType(),
            renderer->GetSlice());
        rulerMgr->SetVisible(true);

        renderer->RequestRender();
    }
}
