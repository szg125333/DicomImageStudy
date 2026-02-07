#pragma once
#include "IViewRenderer.h"
#include "InteractionMode.h"

class ThreeViewController;

class IInteractionStrategy {
public:
    virtual ~IInteractionStrategy() = default;
    virtual void HandleEvent(EventType type, int viewIndex) = 0;
};
