#pragma once
#include "IDistanceMeasureManager.h"
#include <vtkSmartPointer.h>
#include <array>

class vtkRenderer;
class vtkImageViewer2;

/// @brief 距离测量工具管理器实现
class SimpleDistanceMeasureManager : public IDistanceMeasureManager {
public:
    SimpleDistanceMeasureManager();
    ~SimpleDistanceMeasureManager() override;

    void Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) override;
    void StartMeasure(const std::array<double, 3>& startPoint) override;
    void UpdateMeasure(const std::array<double, 3>& endPoint) override;
    void EndMeasure() override;
    void SetVisible(bool visible) override;
    void Shutdown() override;

private:
    vtkSmartPointer<vtkRenderer> m_overlayRenderer;
    vtkImageViewer2* m_viewer = nullptr;
    bool m_initialized = false;
    bool m_visible = true;
};