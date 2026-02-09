#include "SimpleHandIrregularContourManager.h"
#include <vtkRenderer.h>
#include <vtkImageViewer2.h>
#include <QDebug>

SimpleHandIrregularContourManager::SimpleHandIrregularContourManager() = default;

SimpleHandIrregularContourManager::~SimpleHandIrregularContourManager() {
    Shutdown();
}

void SimpleHandIrregularContourManager::Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) {
    if (m_initialized) return;
    if (!overlayRenderer) return;

    m_overlayRenderer = overlayRenderer;
    m_viewer = viewer;
    m_initialized = true;

    qDebug() << "[SimpleHandIrregularContourManager] Initialized";
}

void SimpleHandIrregularContourManager::StartDrawing() {
    qDebug() << "[SimpleHandIrregularContourManager] StartDrawing";
}

void SimpleHandIrregularContourManager::AddPoint(const std::array<double, 3>& point) {
    qDebug() << "[SimpleHandIrregularContourManager] AddPoint - Point:"
        << point[0] << point[1] << point[2];
}

void SimpleHandIrregularContourManager::EndDrawing() {
    qDebug() << "[SimpleHandIrregularContourManager] EndDrawing";
}

void SimpleHandIrregularContourManager::SetVisible(bool visible) {
    m_visible = visible;
    qDebug() << "[SimpleHandIrregularContourManager] SetVisible:" << visible;
}

void SimpleHandIrregularContourManager::Shutdown() {
    if (!m_initialized) return;

    qDebug() << "[SimpleHandIrregularContourManager] Shutdown";
    m_overlayRenderer = nullptr;
    m_viewer = nullptr;
    m_initialized = false;
}