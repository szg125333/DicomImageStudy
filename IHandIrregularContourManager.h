#pragma once
#include <array>

class vtkRenderer;
class vtkImageViewer2;

/// @brief 手工不规则轮廓管理器接口
class IHandIrregularContourManager {
public:
    virtual ~IHandIrregularContourManager() = default;

    virtual void Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) = 0;
    virtual void StartDrawing() = 0;
    virtual void AddPoint(const std::array<double, 3>& point) = 0;
    virtual void EndDrawing() = 0;
    virtual void SetVisible(bool visible) = 0;
    virtual void Shutdown() = 0;
};