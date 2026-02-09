#include "SimpleRegistrationROIManager.h"
#include <vtkRenderer.h>
#include <vtkImageViewer2.h>
#include <QDebug>

SimpleRegistrationROIManager::SimpleRegistrationROIManager() = default;

SimpleRegistrationROIManager::~SimpleRegistrationROIManager() {
    Shutdown();
}

void SimpleRegistrationROIManager::Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) {
    if (m_initialized) return;
    if (!overlayRenderer) return;

    m_overlayRenderer = overlayRenderer;
    m_viewer = viewer;
    m_initialized = true;

    qDebug() << "[SimpleRegistrationROIManager] Initialized";
}

void SimpleRegistrationROIManager::StartSelection() {
    qDebug() << "[SimpleRegistrationROIManager] StartSelection";
}

void SimpleRegistrationROIManager::UpdateROI(const std::array<double, 3>& minPoint,
    const std::array<double, 3>& maxPoint) {
    qDebug() << "[SimpleRegistrationROIManager] UpdateROI - Min:" << minPoint[0] << minPoint[1] << minPoint[2]
        << "Max:" << maxPoint[0] << maxPoint[1] << maxPoint[2];
}

void SimpleRegistrationROIManager::EndSelection() {
    qDebug() << "[SimpleRegistrationROIManager] EndSelection";
}

void SimpleRegistrationROIManager::SetVisible(bool visible) {
    m_visible = visible;
    qDebug() << "[SimpleRegistrationROIManager] SetVisible:" << visible;
}

void SimpleRegistrationROIManager::Shutdown() {
    if (!m_initialized) return;

    qDebug() << "[SimpleRegistrationROIManager] Shutdown";
    m_overlayRenderer = nullptr;
    m_viewer = nullptr;
    m_initialized = false;
}