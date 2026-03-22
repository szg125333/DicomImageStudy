#include "ImageDragStrategy.h"
#include "Interface/IViewController.h"
#include "Interface/IViewRenderer.h"

#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkImageViewer2.h>

#include <cmath>

ImageDragStrategy::ImageDragStrategy(IViewController* controller)
    : IInteractionStrategy(controller)
{
}

// ============================================================
//  事件分发
// ============================================================

void ImageDragStrategy::HandleEvent(EventType        type,
    int              viewIndex,
    const EventData& data)
{
    if (!m_controller) return;

    // 视图锁定：一次拖动只在一个视图内
    if (m_activeViewIndex != -1 && m_activeViewIndex != viewIndex) return;

    auto* renderer = m_controller->GetRenderer(viewIndex);
    if (!renderer) return;

    switch (type) {

        // ── 左键按下：记录起始世界坐标 ──────────────────────────────
    case EventType::LeftPress: {
        auto startWorld = renderer->PickWorldPosition(data.mousePosX, data.mousePosY);
        if (std::isnan(startWorld[0])) break;

        m_isDragging = true;
        m_activeViewIndex = viewIndex;
        m_lastWorldPos = startWorld;
        break;
    }

                             // ── 左键拖动：平移相机 + 累计位移 ───────────────────────────
    case EventType::LeftMove: {
        if (!m_isDragging) break;

        // 当前帧世界坐标
        auto currentWorld = renderer->PickWorldPosition(data.mousePosX, data.mousePosY);
        if (std::isnan(currentWorld[0])) break;

        // 本帧世界坐标增量（mm）
        const double dx = currentWorld[0] - m_lastWorldPos[0];
        const double dy = currentWorld[1] - m_lastWorldPos[1];
        const double dz = currentWorld[2] - m_lastWorldPos[2];

        if (std::abs(dx) < 1e-6 && std::abs(dy) < 1e-6 && std::abs(dz) < 1e-6) break;

        // ── 平移相机（焦点 + 位置同步平移，图像跟随）──────────
        auto viewer = renderer->GetViewer();
        if (!viewer || !viewer->GetRenderer()) break;

        auto* camera = viewer->GetRenderer()->GetActiveCamera();
        if (!camera) break;

        double focal[3], pos[3];
        camera->GetFocalPoint(focal);
        camera->GetPosition(pos);

        // 相机反向移动 → 图像向拖动方向移动
        camera->SetFocalPoint(focal[0] - dx, focal[1] - dy, focal[2] - dz);
        camera->SetPosition(pos[0] - dx, pos[1] - dy, pos[2] - dz);
        viewer->GetRenderer()->ResetCameraClippingRange();

        // 更新基准点
        m_lastWorldPos = currentWorld;

        // ── 累计物理位移 ─────────────────────────────────────
        m_totalDx += dx;
        m_totalDy += dy;
        m_totalDz += dz;

        const double total = std::sqrt(
            m_totalDx * m_totalDx +
            m_totalDy * m_totalDy +
            m_totalDz * m_totalDz);

        if (m_dragCb) {
            m_dragCb(viewIndex, m_totalDx, m_totalDy, m_totalDz, total);
        }

        renderer->RequestRender();
        break;
    }

                            // ── 左键释放：结束拖动 ───────────────────────────────────────
    case EventType::LeftRelease: {
        m_isDragging = false;
        m_activeViewIndex = -1;
        break;
    }

                               // ── 右键单击：清零累计位移 ───────────────────────────────────
    case EventType::RightPress: {
        ResetDisplacement();
        break;
    }

    default:
        break;
    }
}

// ============================================================
//  清除（切换模式时调用）
// ============================================================

void ImageDragStrategy::Clear(int /*viewIndex*/)
{
    m_isDragging = false;
    m_activeViewIndex = -1;
    ResetDisplacement();
}

// ============================================================
//  私有：清零
// ============================================================

void ImageDragStrategy::ResetDisplacement()
{
    m_totalDx = 0.0;
    m_totalDy = 0.0;
    m_totalDz = 0.0;
    if (m_resetCb) m_resetCb();
}
