#include "DistanceMeasureStrategy.h"
#include <QDebug>

DistanceMeasureStrategy::DistanceMeasureStrategy(IViewController* controller)
    : m_controller(controller) {
}

void DistanceMeasureStrategy::HandleEvent(EventType type, int viewIndex, void* data) {
    qDebug() << "[DistanceMeasureStrategy] Event:" << static_cast<int>(type)
        << "ViewIndex:" << viewIndex;
}