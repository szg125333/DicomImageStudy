#include "RegistrationROIStrategy.h"
#include "Interface/IViewController.h"
#include "Interface/IViewRenderer.h"
#include "Renderer/OverlayManager/IOverlayManager.h"
#include "Renderer/OverlayManager/ROIManager/SimpleROIManager.h"

#include <vtkImageViewer2.h>

RegistrationROIStrategy::RegistrationROIStrategy(IViewController* controller)
    : AbstractMeasureStrategy(controller)
{
}

// ============================================================
//  事件分发
// ============================================================

void RegistrationROIStrategy::HandleEvent(EventType        type,
    int              viewIndex,
    const EventData& data)
{
    if (!m_controller) return;

    // 视图锁定：一次绘制只在一个视图内完成
    if (!TryLockView(viewIndex)) return;

    const int screenX = data.mousePosX;
    const int screenY = data.mousePosY;

    // 获取渲染器和 ROI Manager
    auto* renderer = m_controller->GetRenderer(viewIndex);
    if (!renderer) return;

    auto* overlayMgr = renderer->GetOverlayManager();
    if (!overlayMgr) return;

    auto* roiMgr = overlayMgr->GetFeature<SimpleROIManager>();
    if (!roiMgr) return;

    // 获取当前视图方向和切片（用于统计计算和切片跟随）
    ViewType viewType = renderer->GetCurrentViewType();
    int      slice = renderer->GetSlice();

    switch (type) {

        // ---- 左键按下：开始绘制 ----
    case EventType::LeftPress: {
        // 边界检查：起点必须在图像范围内
        if (!IsInsideImage(viewIndex, screenX, screenY)) break;

        auto startWorld = renderer->PickWorldPosition(screenX, screenY);

        m_isDragging = true;
        m_drawingViewIndex = viewIndex;
        m_drawingViewType = viewType;
        m_drawingSlice = slice;

        // 缓存起始角点（AbstractMeasureStrategy 基类字段）
        m_firstWorldPos = startWorld;

        // 通知 Manager 开始一次新 ROI
        roiMgr->BeginROI(startWorld);
        renderer->RequestRender();
        break;
    }

                             // ---- 左键拖拽：实时更新预览框 ----
    case EventType::LeftMove: {
        if (!m_isDragging) break;

        auto currentWorld = renderer->PickWorldPosition(screenX, screenY);
        roiMgr->UpdatePreview(currentWorld);
        renderer->RequestRender();
        break;
    }

                            // ---- 左键释放：完成 ROI ----
    case EventType::LeftRelease: {
        if (!m_isDragging) break;

        m_isDragging = false;

        // 获取 VTK Viewer（传给 Manager 用于读取像素值）
        auto viewer = renderer->GetViewer();
        if (!viewer) {
            roiMgr->CancelCurrentROI();
            UnlockView();
            break;
        }

        auto endWorld = renderer->PickWorldPosition(screenX, screenY);

        // 提交：固定矩形框并计算统计
        roiMgr->CommitROI(endWorld, viewer.Get(),
            m_drawingViewType, m_drawingSlice);
        renderer->RequestRender();
        UnlockView();
        break;
    }

                               // ---- 右键按下：取消当前绘制 ----
    case EventType::RightPress: {
        if (m_isDragging) {
            roiMgr->CancelCurrentROI();
            renderer->RequestRender();
            m_isDragging = false;
            UnlockView();
        }
        break;
    }

                              // ---- 键盘：Delete / Esc / Ctrl+Delete ----
    case EventType::KeyPress: {
        const std::string& key = data.keySym;

        if (key == "Delete" || key == "BackSpace") {
            if (data.ctrlPressed) {
                // Ctrl + Delete：清除所有 ROI
                roiMgr->ClearAllROI();
            }
            else {
                // Delete：删除最后一个 ROI
                roiMgr->DeleteLastROI();
            }
            renderer->RequestRender();
        }
        else if (key == "Escape") {
            // Esc：取消当前未完成的绘制
            if (m_isDragging) {
                roiMgr->CancelCurrentROI();
                renderer->RequestRender();
                m_isDragging = false;
                UnlockView();
            }
        }
        break;
    }

    default:
        break;
    }
}

// ============================================================
//  清除
// ============================================================

void RegistrationROIStrategy::Clear(int viewIndex)
{
    if (!m_controller) return;

    auto* renderer = m_controller->GetRenderer(viewIndex);
    if (!renderer) return;

    auto* overlayMgr = renderer->GetOverlayManager();
    if (!overlayMgr) return;

    auto* roiMgr = overlayMgr->GetFeature<SimpleROIManager>();
    if (roiMgr) {
        roiMgr->ClearAllROI();
    }

    // 重置策略内部状态
    m_isDragging = false;
    m_drawingViewIndex = -1;
    m_drawingViewType = ViewType::None;
    m_drawingSlice = 0;
    UnlockView();
}