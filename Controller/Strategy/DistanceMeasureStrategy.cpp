#include "DistanceMeasureStrategy.h"
#include "Interface/IViewController.h"
#include "Interface/IViewRenderer.h"
#include "Renderer/OverlayManager/DistanceMeasureManager/SimpleDistanceMeasureManager.h"
#include <QDebug>
#include "Renderer/OverlayManager/IOverlayManager.h"

DistanceMeasureStrategy::DistanceMeasureStrategy(IViewController* controller)
    : m_controller(controller)
    , m_startWorldPos({ 0.0, 0.0, 0.0 })
    , m_hasFirstPoint(false)
{
}

void DistanceMeasureStrategy::HandleEvent(EventType type, int viewIndex, const EventData& data) {
    //auto pos = static_cast<int*>(data);
    int pos[2];
    pos[0] = data.mousePosX;
    pos[1] = data.mousePosY;
    if (!pos || !m_controller) return;

    // 获取当前视图的渲染器
    auto renderer = m_controller->GetRenderer(viewIndex);
    if (!renderer) return;

    // 获取 overlay manager
    auto overlayMgr = renderer->GetOverlayManager();
    if (!overlayMgr) return;

    // 获取测距功能模块
    auto distanceFeature = overlayMgr->GetFeature<SimpleDistanceMeasureManager>();
    if (!distanceFeature) return;

    switch (type) {
    case EventType::LeftPress: {
        if (!m_isEditing) {
            // 如果不是在编辑状态，才处理测量的起点和终点
            if (!m_hasFirstPoint) {
                // 第一次点击：记录起始点（世界坐标）
                m_startWorldPos = renderer->PickWorldPosition(pos[0], pos[1]);
                m_startViewIndex = viewIndex;
                m_hasFirstPoint = true;

                distanceFeature->DrawStartPoint(m_startWorldPos);
                renderer->RequestRender(); // 触发重绘
            }
            else {
                // 第二次点击：完成测量
                auto endWorldPos = renderer->PickWorldPosition(pos[0], pos[1]);
                distanceFeature->DrawFinalMeasurementLine(m_startWorldPos, endWorldPos);
                renderer->RequestRender();

                // 重置状态
                m_hasFirstPoint = false;
            }
        }
        else
        {
            auto editablePoint = distanceFeature->GetEditablePoint(pos[0], pos[1]);
            qDebug() << "EditablePoint - MeasurementId:" << editablePoint.measurementId
				<< "IsStart:" << editablePoint.isStart;

            if (editablePoint.measurementId != -1) {
                m_isEditing = true;
                m_editingMeasurementId = editablePoint.measurementId;
                m_editingIsStart = editablePoint.isStart;
                m_editingViewIndex = viewIndex;

                qDebug() << "Start editing measurement" << m_editingMeasurementId
                    << (m_editingIsStart ? "start" : "end") << "point";
            }
        }


        break;
    }

    case EventType::LeftMove: {
        if (!m_isEditing) {
            if (m_hasFirstPoint) {
                // 鼠标移动：预览测量线
                auto currentWorldPos = renderer->PickWorldPosition(pos[0], pos[1]);
                distanceFeature->PreviewMeasurementLine(m_startWorldPos, currentWorldPos);
                renderer->RequestRender();
            }
        }
        else
        {
            // 👉 拖动：拾取新世界坐标并更新
            auto newWorldPos = renderer->PickWorldPosition(data.mousePosX, data.mousePosY);
            distanceFeature->UpdateMeasurementPoint(
                m_editingMeasurementId, m_editingIsStart, newWorldPos);
            renderer->RequestRender(); // 实时刷新
        }

        break;
    }

    case EventType::LeftRelease: {
        if (m_isEditing) {
            // 👉 结束此次编辑
            m_editingMeasurementId = -1;
            m_editingViewIndex = -1;
            qDebug() << "Finish editing measurement";
            // 可选：保存到 undo stack 或触发更新
        }
		break;
    }
    case EventType::RightPress:
    case EventType::RightRelease: {
        if (m_hasFirstPoint) {
            // 右键取消
            distanceFeature->ClearCurrentMeasurement(); // 清除所有绘制
            renderer->RequestRender();
            m_hasFirstPoint = false;
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