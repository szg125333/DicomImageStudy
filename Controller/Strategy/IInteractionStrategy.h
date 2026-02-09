#pragma once
#include "Interface/IViewRenderer.h"
#include "Common/InteractionMode.h"

class IViewController;

class IInteractionStrategy {
public:
    virtual ~IInteractionStrategy() = default;
    virtual void HandleEvent(EventType type, int viewIndex,void* data) = 0;
};
