#include "SimpleCheckerboardManager.h"
#include <vtkRenderer.h>
#include <vtkImageViewer2.h>
#include <QDebug>

SimpleCheckerboardManager::SimpleCheckerboardManager() = default;

SimpleCheckerboardManager::~SimpleCheckerboardManager() {
    Shutdown();
}

void SimpleCheckerboardManager::Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) {
    if (m_initialized) return;
    if (!overlayRenderer) return;

    m_overlayRenderer = overlayRenderer;
    m_viewer = viewer;
    m_initialized = true;

    qDebug() << "[SimpleCheckerboardManager] Initialized";
}

void SimpleCheckerboardManager::SetCheckerboardMode(bool enabled, double ratio) {
    qDebug() << "[SimpleCheckerboardManager] SetCheckerboardMode - Enabled:" << enabled
        << "Ratio:" << ratio;
}

void SimpleCheckerboardManager::SetVisible(bool visible) {
    m_visible = visible;
    qDebug() << "[SimpleCheckerboardManager] SetVisible:" << visible;
}

void SimpleCheckerboardManager::Shutdown() {
    if (!m_initialized) return;

    qDebug() << "[SimpleCheckerboardManager] Shutdown";
    m_overlayRenderer = nullptr;
    m_viewer = nullptr;
    m_initialized = false;
}