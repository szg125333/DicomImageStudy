#pragma once
#include "IHandIrregularContourManager.h"
#include <vtkSmartPointer.h>
#include <array>

class vtkRenderer;
class vtkImageViewer2;

/// @brief 手工不规则轮廓管理器实现
class SimpleHandIrregularContourManager : public IHandIrregularContourManager {
public:
    SimpleHandIrregularContourManager();
    ~SimpleHandIrregularContourManager() override;

    void Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) override;
    void StartDrawing() override;
    void AddPoint(const std::array<double, 3>& point) override;
    void EndDrawing() override;
    void SetVisible(bool visible) override;
    void Shutdown() override;

private:
    vtkSmartPointer<vtkRenderer> m_overlayRenderer;
    vtkImageViewer2* m_viewer = nullptr;
    bool m_initialized = false;
    bool m_visible = true;
};