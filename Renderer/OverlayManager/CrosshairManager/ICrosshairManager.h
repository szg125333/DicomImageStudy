#pragma once
#include "Common/ViewTypes.h"
#include <array>
#include "../IOverlayFeature.h"

class vtkRenderer;

class ICrosshairManager :public IOverlayFeature {
public:
    virtual ~ICrosshairManager() = default;
    //virtual void Initialize(vtkRenderer* overlayRenderer) = 0;

    // 新接口：传入十字中心 worldPoint，视图类型，以及 worldMin/worldMax（每轴）
    virtual void UpdateCrosshair(std::array<double, 3> worldPoint,
        ViewType view,
        const double worldMin[3],
        const double worldMax[3]) = 0;

    //virtual void SetVisible(bool visible) = 0;
    //virtual void SetColor(double r, double g, double b) = 0;
    //virtual void Shutdown() = 0;
};
