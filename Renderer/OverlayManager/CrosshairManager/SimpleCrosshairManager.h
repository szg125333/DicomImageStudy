#pragma once
#include "ICrosshairManager.h"
#include "../IOverlayFeature.h"
#include <vtkSmartPointer.h>

class vtkRenderer;
class vtkLineSource;
class vtkActor;
class vtkPolyDataMapper;

class SimpleCrosshairManager : public ICrosshairManager{
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
    void OnSliceChanged(const vtkImageViewer2* viewer,int slice,ViewType viewType) override;
    void ClearAllMeasurement();
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

    std::array<double, 3> m_lastWorldPoint = { 0, 0, 0 };
    double m_lastImageMin[3] = {0, 0, 0};
    double m_lastImageMax[3] = {0, 0, 0};
    bool m_hasValidPoint = false;
};
