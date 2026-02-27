#include "HandIrregularContourStrategy.h"
#include <QDebug>

HandIrregularContourStrategy::HandIrregularContourStrategy(IViewController* controller)
    : m_controller(controller) {
}

void HandIrregularContourStrategy::HandleEvent(EventType type, int viewIndex, const EventData& data) {
    qDebug() << "[HandIrregularContourStrategy] Event:" << static_cast<int>(type)
        << "ViewIndex:" << viewIndex;
}