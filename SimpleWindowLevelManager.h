#pragma once
#include "IWindowLevelManager.h"
#include <vtkSmartPointer.h>
#include <vtkImageViewer2.h>

class vtkRenderer;
class vtkTextActor;

class SimpleWindowLevelManager : public IWindowLevelManager {
public:
    SimpleWindowLevelManager();
    ~SimpleWindowLevelManager() override;

    void Initialize(vtkRenderer* overlayRenderer,vtkImageViewer2* viewer) override;
    void SetWindowLevel(double ww, double wl) override;
    void SetVisible(bool visible) override;
    void Shutdown() override;

private:
    vtkSmartPointer<vtkRenderer> m_overlayRenderer;
    vtkSmartPointer<vtkImageViewer2> m_viewer;
    vtkSmartPointer<vtkTextActor> m_textActor;
    bool m_initialized = false;
    bool m_visible = true;
    double m_ww = 0.0;
    double m_wl = 0.0;
};
