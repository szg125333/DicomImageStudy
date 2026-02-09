#pragma once
#include <array>

class vtkRenderer;
class vtkImageViewer2;

/// @brief 角度测量工具管理器接口
class IAngleMeasureManager {
public:
    virtual ~IAngleMeasureManager() = default;

    virtual void Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) = 0;
    virtual void StartMeasure(const std::array<double, 3>& point1) = 0;
    virtual void UpdateMeasure(const std::array<double, 3>& point2) = 0;
    virtual void EndMeasure(const std::array<double, 3>& point3) = 0;
    virtual void SetVisible(bool visible) = 0;
    virtual void Shutdown() = 0;
};