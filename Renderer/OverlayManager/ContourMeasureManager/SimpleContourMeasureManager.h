#pragma once
#include "IContourMeasureManager.h"
#include <vtkSmartPointer.h>
#include <array>

class vtkRenderer;
class vtkImageViewer2;

/// @brief 轮廓测量管理器实现
class SimpleContourMeasureManager : public IContourMeasureManager {
public:
    SimpleContourMeasureManager();
    ~SimpleContourMeasureManager() override;

    void Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) override;
    void StartMeasure() override;
    void AddPoint(const std::array<double, 3>& point) override;
    void EndMeasure() override;
    void SetVisible(bool visible) override;
    void Shutdown() override;

private:
    vtkSmartPointer<vtkRenderer> m_overlayRenderer;
    vtkImageViewer2* m_viewer = nullptr;
    bool m_initialized = false;
    bool m_visible = true;
};