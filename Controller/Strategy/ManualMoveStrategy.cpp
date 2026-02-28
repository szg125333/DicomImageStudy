#include "ManualMoveStrategy.h"
#include <QDebug>

ManualMoveStrategy::ManualMoveStrategy(IViewController* controller)
    : IInteractionStrategy(controller) {
}

void ManualMoveStrategy::HandleEvent(EventType type, int viewIndex, const EventData& data) {
    qDebug() << "[ManualMoveStrategy] Event:" << static_cast<int>(type)
        << "ViewIndex:" << viewIndex;
}