#include "SimpleDistanceMeasureManager.h"
#include <vtkRenderer.h>
#include <vtkImageViewer2.h>
#include <QDebug>

SimpleDistanceMeasureManager::SimpleDistanceMeasureManager() = default;

SimpleDistanceMeasureManager::~SimpleDistanceMeasureManager() {
    Shutdown();
}

void SimpleDistanceMeasureManager::Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) {
    if (m_initialized) return;
    if (!overlayRenderer) return;

    m_overlayRenderer = overlayRenderer;
    m_viewer = viewer;
    m_initialized = true;

    qDebug() << "[SimpleDistanceMeasureManager] Initialized";
}

void SimpleDistanceMeasureManager::StartMeasure(const std::array<double, 3>& startPoint) {
    qDebug() << "[SimpleDistanceMeasureManager] StartMeasure - Point:"
        << startPoint[0] << startPoint[1] << startPoint[2];
}

void SimpleDistanceMeasureManager::UpdateMeasure(const std::array<double, 3>& endPoint) {
    qDebug() << "[SimpleDistanceMeasureManager] UpdateMeasure - Point:"
        << endPoint[0] << endPoint[1] << endPoint[2];
}

void SimpleDistanceMeasureManager::EndMeasure() {
    qDebug() << "[SimpleDistanceMeasureManager] EndMeasure";
}

void SimpleDistanceMeasureManager::SetVisible(bool visible) {
    m_visible = visible;
    qDebug() << "[SimpleDistanceMeasureManager] SetVisible:" << visible;
}

void SimpleDistanceMeasureManager::Shutdown() {
    if (!m_initialized) return;

    qDebug() << "[SimpleDistanceMeasureManager] Shutdown";
    m_overlayRenderer = nullptr;
    m_viewer = nullptr;
    m_initialized = false;
}