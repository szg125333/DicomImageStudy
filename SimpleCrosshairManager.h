#pragma once
#include "ICrosshairManager.h"
#include <vtkSmartPointer.h>

class vtkRenderer;
class vtkLineSource;
class vtkActor;
class vtkPolyDataMapper;

class SimpleCrosshairManager : public ICrosshairManager {
public:
    SimpleCrosshairManager();
    ~SimpleCrosshairManager() override;

    void Initialize(vtkRenderer* overlayRenderer) override;
    void UpdateCrosshair(std::array<double, 3> worldPoint,
        ViewType view,
        const double worldMin[3],
        const double worldMax[3]) override;
    void SetVisible(bool visible) override;
    void SetColor(double r, double g, double b) override;
    void Shutdown() override;

private:
    vtkSmartPointer<vtkRenderer> m_overlayRenderer;
    vtkSmartPointer<vtkLineSource> m_hLine;
    vtkSmartPointer<vtkLineSource> m_vLine;
    vtkSmartPointer<vtkPolyDataMapper> m_hMapper;
    vtkSmartPointer<vtkPolyDataMapper> m_vMapper;
    vtkSmartPointer<vtkActor> m_hActor;
    vtkSmartPointer<vtkActor> m_vActor;
    bool m_initialized = false;
    bool m_visible = true;
};
