#include "SimpleAngleMeasureManager.h"
#include <vtkRenderer.h>
#include <vtkImageViewer2.h>
#include <QDebug>

SimpleAngleMeasureManager::SimpleAngleMeasureManager() = default;

SimpleAngleMeasureManager::~SimpleAngleMeasureManager() {
    Shutdown();
}

void SimpleAngleMeasureManager::Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) {
    if (m_initialized) return;
    if (!overlayRenderer) return;

    m_overlayRenderer = overlayRenderer;
    m_viewer = viewer;
    m_initialized = true;

    qDebug() << "[SimpleAngleMeasureManager] Initialized";
}

void SimpleAngleMeasureManager::StartMeasure(const std::array<double, 3>& point1) {
    qDebug() << "[SimpleAngleMeasureManager] StartMeasure - Point1:"
        << point1[0] << point1[1] << point1[2];
}

void SimpleAngleMeasureManager::UpdateMeasure(const std::array<double, 3>& point2) {
    qDebug() << "[SimpleAngleMeasureManager] UpdateMeasure - Point2:"
        << point2[0] << point2[1] << point2[2];
}

void SimpleAngleMeasureManager::EndMeasure(const std::array<double, 3>& point3) {
    qDebug() << "[SimpleAngleMeasureManager] EndMeasure - Point3:"
        << point3[0] << point3[1] << point3[2];
}

void SimpleAngleMeasureManager::SetVisible(bool visible) {
    m_visible = visible;
    qDebug() << "[SimpleAngleMeasureManager] SetVisible:" << visible;
}

void SimpleAngleMeasureManager::Shutdown() {
    if (!m_initialized) return;

    qDebug() << "[SimpleAngleMeasureManager] Shutdown";
    m_overlayRenderer = nullptr;
    m_viewer = nullptr;
    m_initialized = false;
}