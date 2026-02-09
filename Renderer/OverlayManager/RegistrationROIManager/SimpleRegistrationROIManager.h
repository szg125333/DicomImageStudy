#pragma once
#include "IRegistrationROIManager.h"
#include <vtkSmartPointer.h>
#include <array>

class vtkRenderer;
class vtkImageViewer2;

/// @brief 配准 ROI（盒子）管理器实现
class SimpleRegistrationROIManager : public IRegistrationROIManager {
public:
    SimpleRegistrationROIManager();
    ~SimpleRegistrationROIManager() override;

    void Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) override;
    void StartSelection() override;
    void UpdateROI(const std::array<double, 3>& minPoint,
        const std::array<double, 3>& maxPoint) override;
    void EndSelection() override;
    void SetVisible(bool visible) override;
    void Shutdown() override;

private:
    vtkSmartPointer<vtkRenderer> m_overlayRenderer;
    vtkImageViewer2* m_viewer = nullptr;
    bool m_initialized = false;
    bool m_visible = true;
};