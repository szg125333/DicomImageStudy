#include "HandIrregularContourStrategy.h"
#include <QDebug>

HandIrregularContourStrategy::HandIrregularContourStrategy(IViewController* controller)
    : IInteractionStrategy(controller) {
}

void HandIrregularContourStrategy::HandleEvent(EventType type, int viewIndex, const EventData& data) {
    qDebug() << "[HandIrregularContourStrategy] Event:" << static_cast<int>(type)
        << "ViewIndex:" << viewIndex;
}