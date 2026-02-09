#include "SimpleContourMeasureManager.h"
#include <vtkRenderer.h>
#include <vtkImageViewer2.h>
#include <QDebug>

SimpleContourMeasureManager::SimpleContourMeasureManager() = default;

SimpleContourMeasureManager::~SimpleContourMeasureManager() {
    Shutdown();
}

void SimpleContourMeasureManager::Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) {
    if (m_initialized) return;
    if (!overlayRenderer) return;

    m_overlayRenderer = overlayRenderer;
    m_viewer = viewer;
    m_initialized = true;

    qDebug() << "[SimpleContourMeasureManager] Initialized";
}

void SimpleContourMeasureManager::StartMeasure() {
    qDebug() << "[SimpleContourMeasureManager] StartMeasure";
}

void SimpleContourMeasureManager::AddPoint(const std::array<double, 3>& point) {
    qDebug() << "[SimpleContourMeasureManager] AddPoint - Point:"
        << point[0] << point[1] << point[2];
}

void SimpleContourMeasureManager::EndMeasure() {
    qDebug() << "[SimpleContourMeasureManager] EndMeasure";
}

void SimpleContourMeasureManager::SetVisible(bool visible) {
    m_visible = visible;
    qDebug() << "[SimpleContourMeasureManager] SetVisible:" << visible;
}

void SimpleContourMeasureManager::Shutdown() {
    if (!m_initialized) return;

    qDebug() << "[SimpleContourMeasureManager] Shutdown";
    m_overlayRenderer = nullptr;
    m_viewer = nullptr;
    m_initialized = false;
}