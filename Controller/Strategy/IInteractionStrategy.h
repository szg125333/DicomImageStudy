#pragma once
#include "Interface/IViewRenderer.h"
#include "Common/InteractionMode.h"
#include "Common/EventData.h"

class IViewController;

class IInteractionStrategy {
public:
    virtual ~IInteractionStrategy() = default;
    virtual void HandleEvent(EventType type, int viewIndex,const EventData& data) = 0;
};
