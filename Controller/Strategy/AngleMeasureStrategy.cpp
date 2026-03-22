#include "AngleMeasureStrategy.h"
#include "Interface/IViewController.h"
#include "Interface/IViewRenderer.h"
#include "Renderer/OverlayManager/IOverlayManager.h"
#include "Renderer/OverlayManager/AngleMeasureManager/SimpleAngleMeasureManager.h"

AngleMeasureStrategy::AngleMeasureStrategy(IViewController* controller)
    : AbstractMeasureStrategy(controller)
{
}

// ============================================================
//  事件分发
// ============================================================

void AngleMeasureStrategy::HandleEvent(EventType type,
    int viewIndex,
    const EventData& data)
{
    if (!m_controller) return;

    const int screenX = data.mousePosX;
    const int screenY = data.mousePosY;

    // 边界检查：点击到图像外直接忽略
    if (!IsInsideImage(viewIndex, screenX, screenY)) return;

    auto* renderer = m_controller->GetRenderer(viewIndex);
    if (!renderer) return;

    auto* overlayMgr = renderer->GetOverlayManager();
    if (!overlayMgr) return;

    auto* angleFeature = overlayMgr->GetFeature<SimpleAngleMeasureManager>();
    if (!angleFeature) return;

    switch (type) {

        // ---- 左键点击：逐步放置三个端点 ----
    case EventType::LeftPress: {
        if (!m_isEditingExisting) {
            auto worldPos = renderer->PickWorldPosition(screenX, screenY);

            if (m_step == MeasureStep::Idle) {
                // 视图锁定：一次测量只在一个视图内完成
                if (!TryLockView(viewIndex)) return;

                // 第一次点击：放置起始点
                m_firstWorldPos = worldPos;
                m_step = MeasureStep::StartPlaced;
                angleFeature->DrawStartPoint(m_firstWorldPos);
                renderer->RequestRender();
            }
            else if (m_step == MeasureStep::StartPlaced) {
                // 第二次点击：放置顶点
                m_vertexWorldPos = worldPos;
                m_step = MeasureStep::VertexPlaced;
                angleFeature->DrawMiddlePointAndStartToMiddleLine(m_vertexWorldPos);
                renderer->RequestRender();
            }
            else if (m_step == MeasureStep::VertexPlaced) {
                // 第三次点击：放置终止点，完成测量
                angleFeature->DrawEndPointAndMiddleToEndLine(worldPos);
                renderer->RequestRender();

                // 重置状态，允许继续下一次测量
                m_step = MeasureStep::Idle;
                UnlockView();
            }
        }
        else {
            // 编辑模式：拾取可拖拽端点
            auto editPoint = angleFeature->GetEditableAnglePoint(screenX, screenY);
            if (editPoint.measurementId != -1) {
                m_editingMeasurementId = editPoint.measurementId;
                m_editingPointRole = static_cast<int>(editPoint.role);
            }
        }
        break;
    }

                             // ---- 鼠标移动：预览线段 / 拖拽端点 ----
    case EventType::LeftMove: {
        if (!m_isEditingExisting) {
            auto currentPos = renderer->PickWorldPosition(screenX, screenY);

            if (m_step == MeasureStep::StartPlaced) {
                // 预览起始点 → 当前位置的连线
                angleFeature->PreviewStartToMiddleMeasurementLine(
                    m_firstWorldPos, currentPos);
                renderer->RequestRender();
            }
            else if (m_step == MeasureStep::VertexPlaced) {
                // 预览顶点 → 当前位置的连线
                angleFeature->PreviewMiddleToEndMeasurementLine(
                    m_vertexWorldPos, currentPos);
                renderer->RequestRender();
            }
        }
        else if (m_editingMeasurementId != -1) {
            // 拖拽已有端点
            auto newPos = renderer->PickWorldPosition(screenX, screenY);
            angleFeature->UpdateAngleMeasurementPoint(
                m_editingMeasurementId, static_cast<AnglePointRole>(m_editingPointRole), newPos);
            renderer->RequestRender();
        }
        break;
    }

                            // ---- 左键释放：结束拖拽编辑 ----
    case EventType::LeftRelease: {
        if (m_isEditingExisting) {
            m_editingMeasurementId = -1;
            m_editingPointRole = -1;
            UnlockView();
        }
        break;
    }

                               // ---- 右键：取消当前未完成的测量 ----
    case EventType::RightPress: {
        if (m_step != MeasureStep::Idle) {
            angleFeature->ClearCurrentMeasurement();
            renderer->RequestRender();
            m_step = MeasureStep::Idle;
            UnlockView();
        }
        break;
    }

                              // ---- 键盘：Ctrl 键控制编辑模式 ----
    case EventType::KeyPress:
        m_isEditingExisting = data.ctrlPressed;
        break;

    case EventType::KeyRelease:
        m_isEditingExisting = data.ctrlPressed;
        if (!m_isEditingExisting) {
            m_editingMeasurementId = -1;
            m_editingPointRole = -1;
            UnlockView();
        }
        break;

    default:
        break;
    }
}

// ============================================================
//  清除
// ============================================================

void AngleMeasureStrategy::Clear(int viewIndex)
{
    if (!m_controller) return;

    auto* renderer = m_controller->GetRenderer(viewIndex);
    if (!renderer) return;

    auto* overlayMgr = renderer->GetOverlayManager();
    if (!overlayMgr) return;

    auto* angleFeature = overlayMgr->GetFeature<SimpleAngleMeasureManager>();
    if (angleFeature) {
        angleFeature->ClearAllMeasurement();
    }

    // 重置策略内部状态
    m_step = MeasureStep::Idle;
    m_isEditingExisting = false;
    m_editingMeasurementId = -1;
    m_editingPointRole = -1;
    UnlockView();
}
