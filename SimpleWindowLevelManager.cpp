#include "SimpleWindowLevelManager.h"
#include <vtkRenderer.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <QString>

SimpleWindowLevelManager::SimpleWindowLevelManager() = default;
SimpleWindowLevelManager::~SimpleWindowLevelManager() { Shutdown(); }

void SimpleWindowLevelManager::Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) {
    if (m_initialized || !overlayRenderer||!viewer) return;
    m_overlayRenderer = overlayRenderer;
    m_viewer = viewer;

    m_textActor = vtkSmartPointer<vtkTextActor>::New();
    m_textActor->GetTextProperty()->SetColor(1.0, 1.0, 0.0);
    m_textActor->GetTextProperty()->SetFontSize(12);
    m_textActor->SetDisplayPosition(10, 10);
    m_overlayRenderer->AddActor2D(m_textActor);

    m_initialized = true;
    m_visible = true;
}

void SimpleWindowLevelManager::SetWindowLevel(double ww, double wl) {
    if (!m_initialized) {
        m_ww = ww; m_wl = wl; // 缓存，等初始化时再显示
        return;
    }
    m_ww = ww; m_wl = wl;
    QString txt = QString("WW:%1 WL:%2").arg(ww).arg(wl);
    m_textActor->SetInput(txt.toStdString().c_str());
    m_textActor->SetVisibility(m_visible ? 1 : 0);
    m_viewer->SetColorWindow(ww);
    m_viewer->SetColorLevel(wl);
    // 不直接 Render，交给上层合并渲染
}

void SimpleWindowLevelManager::SetVisible(bool visible) {
    m_visible = visible;
    if (!m_initialized) return;
    m_textActor->SetVisibility(visible ? 1 : 0);
}

void SimpleWindowLevelManager::Shutdown() {
    if (!m_initialized) return;
    if (m_overlayRenderer && m_textActor) {
        m_overlayRenderer->RemoveActor2D(m_textActor);
    }
    m_textActor = nullptr;
    m_overlayRenderer = nullptr;
    m_initialized = false;
}
