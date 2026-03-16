#include "AngleMeasureStrategy.h"
#include "Interface/IViewController.h"
#include "Renderer/OverlayManager/AngleMeasureManager/SimpleAngleMeasureManager.h"
#include "Renderer/OverlayManager/IOverlayManager.h"
#include <QDebug>

AngleMeasureStrategy::AngleMeasureStrategy(IViewController* controller)
    : IInteractionStrategy(controller) {
}

void AngleMeasureStrategy::HandleEvent(EventType type, int viewIndex, const EventData& data) {
    int pos[2];
    pos[0] = data.mousePosX;
    pos[1] = data.mousePosY;

    if (!pos || !m_controller) return;

    if (m_editingViewIndex != viewIndex && m_editingViewIndex != -1) {
        return; // 只处理当前正在编辑的视图事件
    }

    // 获取当前视图的渲染器
    auto renderer = m_controller->GetRenderer(viewIndex);
    if (!renderer) return;

    // 获取 overlay manager
    auto overlayMgr = renderer->GetOverlayManager();
    if (!overlayMgr) return;

    // 获取测距功能模块
    auto angleFeature = overlayMgr->GetFeature<SimpleAngleMeasureManager>();
    if (!angleFeature) return;

    bool flag = overlayMgr->IsWorldPointInImage(renderer->PickWorldPosition(pos[0], pos[1]));
    if (!flag)   // 如果点击位置不在图像范围内，直接忽略事件
        return;

    switch (type) {
    case EventType::LeftPress: {
        if (!m_isEditing) {
            if (m_editStatus==EditStatus::None) {
                // 第一次点击：记录起始点（世界坐标）
                m_startWorldPos = renderer->PickWorldPosition(pos[0], pos[1]);
                m_startViewIndex = viewIndex;
                m_editStatus = EditStatus::startPonit;
                m_editingViewIndex = viewIndex;

                angleFeature->DrawStartPoint(m_startWorldPos);
                renderer->RequestRender(); // 触发重绘
            }
            else if(m_editStatus == EditStatus::startPonit){
                // 第二次点击：完成测量
                auto middleWorldPos = renderer->PickWorldPosition(pos[0], pos[1]);
                m_middleWorldPos = middleWorldPos;
                angleFeature->DrawMiddlePointAndStartToMiddleLine(middleWorldPos);
                renderer->RequestRender();
                m_editingViewIndex = viewIndex;

                // 重置状态
                m_editStatus = EditStatus::middlePoint;
            }
            else
            {
                auto endWorldPos = renderer->PickWorldPosition(pos[0], pos[1]);
                angleFeature->DrawEndPointAndMiddleToEndLine(endWorldPos);
                renderer->RequestRender();
                m_editStatus = EditStatus::None;
                m_editingViewIndex = -1;
            }
        }
        else
        {
            m_currentEditablePoint = angleFeature->GetEditableAnglePoint(pos[0], pos[1]);

            if (m_currentEditablePoint.measurementId != -1) {
                m_isEditing = true;
                m_editingMeasurementId = m_currentEditablePoint.measurementId;
            }
        }
        break;
    }

    case EventType::LeftMove: {
        if (!m_isEditing) {
            if (m_editStatus == EditStatus::startPonit) {
                // 鼠标移动：预览测量线
                auto currentWorldPos = renderer->PickWorldPosition(pos[0], pos[1]);
                angleFeature->PreviewStartToMiddleMeasurementLine(m_startWorldPos, currentWorldPos);
                renderer->RequestRender();
            }
            else if(m_editStatus == EditStatus::middlePoint)
            {
                // 鼠标移动：预览测量线
                auto currentWorldPos = renderer->PickWorldPosition(pos[0], pos[1]);
                angleFeature->PreviewMiddleToEndMeasurementLine(m_middleWorldPos,currentWorldPos);
                renderer->RequestRender();
            }
        }
        else
        {
            // 👉 拖动：拾取新世界坐标并更新
            auto newWorldPos = renderer->PickWorldPosition(data.mousePosX, data.mousePosY);
            angleFeature->UpdateAngleMeasurementPoint(
                m_editingMeasurementId, m_currentEditablePoint.role, newWorldPos);
            renderer->RequestRender(); // 实时刷新
        }

        break;
    }

    case EventType::LeftRelease: {
        if (m_isEditing) {
            // 👉 结束此次编辑
            m_editingMeasurementId = -1;
            m_editingViewIndex = -1;
        }
        break;
    }
    case EventType::RightPress:
    case EventType::RightRelease: {
        if (m_editStatus != EditStatus::None) {
            // 右键取消
            angleFeature->ClearCurrentMeasurement(); // 清除所有绘制
            renderer->RequestRender();
            m_editStatus = EditStatus::None;
        }
        break;
    }

    case EventType::KeyPress:
        m_isEditing = data.ctrlPressed;
        qDebug() << "KeyPress event received. Entering editing mode.";
        break;
    case EventType::KeyRelease:
        m_isEditing = data.ctrlPressed;
        if (!m_isEditing) {
            m_isEditing = false;
            m_editingMeasurementId = -1;
            m_editingViewIndex = -1;
            qDebug() << "Finish editing measurement";
        }
        qDebug() << "KeyRelease event received. Exiting editing mode.";
        break;

    default:
        break;
    }
}

void AngleMeasureStrategy::Clear(int viewIndex)
{
    auto renderer = m_controller->GetRenderer(viewIndex);
    if (!renderer) return;

    // 获取 overlay manager
    auto overlayMgr = renderer->GetOverlayManager();
    if (!overlayMgr) return;

    // 获取测距功能模块
    auto feature = overlayMgr->GetFeature<SimpleAngleMeasureManager>();
    if (!feature) return;

    feature->ClearAllMeasurement(); // 清除所有绘制
}
