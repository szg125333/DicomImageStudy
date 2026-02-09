#include "ContourMeasureStrategy.h"
#include <QDebug>

ContourMeasureStrategy::ContourMeasureStrategy(IViewController* controller)
    : m_controller(controller) {
}

void ContourMeasureStrategy::HandleEvent(EventType type, int viewIndex, void* data) {
    qDebug() << "[ContourMeasureStrategy] Event:" << static_cast<int>(type)
        << "ViewIndex:" << viewIndex;
}