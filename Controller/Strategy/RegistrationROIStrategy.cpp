#include "RegistrationROIStrategy.h"
#include <QDebug>

RegistrationROIStrategy::RegistrationROIStrategy(IViewController* controller)
    : IInteractionStrategy(controller) {
}

void RegistrationROIStrategy::HandleEvent(EventType type, int viewIndex, const EventData& data) {
    qDebug() << "[RegistrationROIStrategy] Event:" << static_cast<int>(type)
        << "ViewIndex:" << viewIndex;
}