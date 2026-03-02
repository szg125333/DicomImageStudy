#pragma once

#include "../IOverlayFeature.h"
#include "Common/ViewTypes.h"
#include "Common/RenderViewState.h"


class vtkRenderer;

class IOverlayInfoManager : public IOverlayFeature {
public:
    virtual ~IOverlayInfoManager() = default;
    virtual void Update(const RenderViewState& state) = 0;
};