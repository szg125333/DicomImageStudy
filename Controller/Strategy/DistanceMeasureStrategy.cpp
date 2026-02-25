#include "DistanceMeasureStrategy.h"
#include "Interface/IViewController.h"
#include "Interface/IViewRenderer.h"
#include "Renderer/OverlayManager/DistanceMeasureManager/SimpleDistanceMeasureManager.h"
#include <QDebug>
#include "Renderer/OverlayManager/IOverlayManager.h"

DistanceMeasureStrategy::DistanceMeasureStrategy(IViewController* controller)
    : m_controller(controller) {
}

void DistanceMeasureStrategy::HandleEvent(EventType type, int viewIndex, void* data) {
    auto pos = static_cast<int*>(data);
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
        if (!m_hasFirstPoint) {
            // 第一次点击：记录起始点（世界坐标）
            m_startWorldPos = renderer->PickWorldPosition(pos[0], pos[1]);
            m_startViewIndex = viewIndex;
            m_hasFirstPoint = true;

            // 👇 直接绘制起点（无需回调 Controller！）
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
        break;
    }

    case EventType::LeftMove: {
        if (m_hasFirstPoint) {
            // 鼠标移动：预览测量线
            auto currentWorldPos = renderer->PickWorldPosition(pos[0], pos[1]);
            distanceFeature->PreviewMeasurementLine(m_startWorldPos, currentWorldPos);
            renderer->RequestRender();
        }
        break;
    }

    case EventType::RightPress:
    case EventType::RightRelease: {
        if (m_hasFirstPoint) {
            // 右键取消
            //distanceFeature->ClearAll(); // 清除所有绘制
            renderer->RequestRender();
            m_hasFirstPoint = false;
        }
        break;
    }

    default:
        break;
    }
}