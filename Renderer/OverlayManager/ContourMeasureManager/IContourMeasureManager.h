#pragma once
#include <array>

class vtkRenderer;
class vtkImageViewer2;

/// @brief 轮廓测量管理器接口
class IContourMeasureManager {
public:
    virtual ~IContourMeasureManager() = default;

    virtual void Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) = 0;
    virtual void StartMeasure() = 0;
    virtual void AddPoint(const std::array<double, 3>& point) = 0;
    virtual void EndMeasure() = 0;
    virtual void SetVisible(bool visible) = 0;
    virtual void Shutdown() = 0;
};