#include "CheckboardStrategy.h"
#include <QDebug>

CheckboardStrategy::CheckboardStrategy(IViewController* controller)
    : m_controller(controller) {
}

void CheckboardStrategy::HandleEvent(EventType type, int viewIndex, const EventData& data) {
    qDebug() << "[CheckboardStrategy] Event:" << static_cast<int>(type)
        << "ViewIndex:" << viewIndex;
}