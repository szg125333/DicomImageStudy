#pragma once
#include "IAngleMeasureManager.h"
#include <vtkSmartPointer.h>
#include <array>

class vtkRenderer;
class vtkImageViewer2;

/// @brief 角度测量工具管理器实现
class SimpleAngleMeasureManager : public IAngleMeasureManager {
public:
    SimpleAngleMeasureManager();
    ~SimpleAngleMeasureManager() override;

    void Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) override;
    void StartMeasure(const std::array<double, 3>& point1) override;
    void UpdateMeasure(const std::array<double, 3>& point2) override;
    void EndMeasure(const std::array<double, 3>& point3) override;
    void SetVisible(bool visible) override;
    void Shutdown() override;

private:
    vtkSmartPointer<vtkRenderer> m_overlayRenderer;
    vtkImageViewer2* m_viewer = nullptr;
    bool m_initialized = false;
    bool m_visible = true;
};