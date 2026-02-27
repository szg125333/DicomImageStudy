#include "AngleMeasureStrategy.h"
#include <QDebug>

AngleMeasureStrategy::AngleMeasureStrategy(IViewController* controller)
    : m_controller(controller) {
}

void AngleMeasureStrategy::HandleEvent(EventType type, int viewIndex, const EventData& data) {
    qDebug() << "[AngleMeasureStrategy] Event:" << static_cast<int>(type)
        << "ViewIndex:" << viewIndex;
}