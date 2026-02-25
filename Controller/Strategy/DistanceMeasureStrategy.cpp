#include "DistanceMeasureStrategy.h"
#include "Interface/IViewController.h"
#include <QDebug>

DistanceMeasureStrategy::DistanceMeasureStrategy(IViewController* controller)
    : m_controller(controller) {
}

void DistanceMeasureStrategy::HandleEvent(EventType type, int viewIndex, void* data) {
    int* pos = static_cast<int*>(data);

    switch (type) {
    case EventType::LeftPress: {
        if (!pos || !m_controller) return;

        if (!m_hasFirstPoint) {
            m_firstPoint[0] = pos[0];
            m_firstPoint[1] = pos[1];
            m_firstViewIndex = viewIndex;
            m_hasFirstPoint = true;

            m_controller->OnDistanceMeasurementStart(viewIndex, pos);
        }
        else {
            m_controller->OnDistanceMeasurementComplete(
                m_firstViewIndex, m_firstPoint,
                viewIndex, pos
            );
            m_hasFirstPoint = false; // 重置状态
        }
        break;
    }
    case EventType::LeftMove: {
        if (m_hasFirstPoint) {
            // 实时通知 controller：鼠标移到了这里，请更新预览线
            m_controller->OnDistancePreview(
                m_firstViewIndex, m_firstPoint,
                viewIndex, pos
            );
        }
		break;
    }

    case EventType::RightPress:
    case EventType::RightRelease: {
        if (m_hasFirstPoint && m_controller) {
            m_controller->OnDistanceMeasurementCancel();
            m_hasFirstPoint = false;
        }
        break;
    }

    default:
        break;
    }
}