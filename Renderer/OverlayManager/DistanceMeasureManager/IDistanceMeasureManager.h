#pragma once
#include <array>

class vtkRenderer;
class vtkImageViewer2;

/// @brief 距离测量工具管理器接口
class IDistanceMeasureManager {
public:
    virtual ~IDistanceMeasureManager() = default;

    virtual void Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) = 0;
    virtual void StartMeasure(const std::array<double, 3>& startPoint) = 0;
    virtual void UpdateMeasure(const std::array<double, 3>& endPoint) = 0;
    virtual void EndMeasure() = 0;
    virtual void SetVisible(bool visible) = 0;
    virtual void Shutdown() = 0;
};