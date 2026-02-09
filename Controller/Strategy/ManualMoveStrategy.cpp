#include "ManualMoveStrategy.h"
#include <QDebug>

ManualMoveStrategy::ManualMoveStrategy(IViewController* controller)
    : m_controller(controller) {
}

void ManualMoveStrategy::HandleEvent(EventType type, int viewIndex, void* data) {
    qDebug() << "[ManualMoveStrategy] Event:" << static_cast<int>(type)
        << "ViewIndex:" << viewIndex;
}