#include "Renderer/OverlayManager/SimpleOverlayManager.h"
#include "Renderer/OverlayManager/CrosshairManager/SimpleCrosshairManager.h"
#include "Renderer/OverlayManager/WindowLevelManager/SimpleWindowLevelManager.h"
#include "Renderer/OverlayManager/DistanceMeasureManager/SimpleDistanceMeasureManager.h"
#include <vtkRenderer.h>
#include <vtkImageViewer2.h>
#include <vtkImageData.h>
#include <vtkCellPicker.h>
#include <QString>

#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkProperty.h>

SimpleOverlayManager::SimpleOverlayManager() = default;

SimpleOverlayManager::~SimpleOverlayManager() {
    Shutdown();
}

void SimpleOverlayManager::Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) {
    if (m_initialized) return;
    if (!overlayRenderer) return;

    m_overlayRenderer = overlayRenderer;
    m_viewer = viewer;

    // 👇 对每一个 feature 调用 Initialize
    for (auto& feature : m_features) {
        if (feature) {
            feature->Initialize(m_overlayRenderer); // 或只传 renderer，依接口而定
        }
    }
    // 应用初始样式/可见性
    SetVisible(m_visible);
    SetColor(m_color[0], m_color[1], m_color[2]);

    m_initialized = true;
}

void SimpleOverlayManager::RegisterFeature(std::unique_ptr<IOverlayFeature> feature) {
    if (!feature) return;

    // 如果已经初始化了，就立即初始化这个新 feature
    if (m_initialized && m_overlayRenderer) {
        feature->Initialize(m_overlayRenderer);
    }
    m_features.push_back(std::move(feature));
}

void SimpleOverlayManager::SetVisible(bool visible) {
    m_visible = visible;
    if (!m_initialized) return;
}

void SimpleOverlayManager::SetColor(double r, double g, double b) {
    m_color[0] = r; m_color[1] = g; m_color[2] = b;
    if (!m_initialized) return;
}

void SimpleOverlayManager::Shutdown() {
    if (!m_initialized) return;

    m_viewer = nullptr;
    m_overlayRenderer = nullptr;
    m_initialized = false;
}

void SimpleOverlayManager::SetImageWorldBounds(const std::array<double, 6>& bounds)
{
    m_imageWorldBounds = bounds;
    m_hasImageBounds = true;
    for (auto& feature : m_features) {
        if (feature) {
            feature->SetImageWorldBounds(bounds); // 或只传 renderer，依接口而定
        }
    }
}

bool SimpleOverlayManager::IsWorldPointInImage(const std::array<double, 3>& worldPoint) const
{
    if (!m_hasImageBounds) {
        return true; // 无边界信息时保守允许
    }

    const double eps = 1e-3; // 容差
    const auto& b = m_imageWorldBounds;
    const auto& p = worldPoint;

    return (p[0] >= b[0] - eps && p[0] <= b[1] + eps &&
        p[1] >= b[2] - eps && p[1] <= b[3] + eps &&
        p[2] >= b[4] - eps && p[2] <= b[5] + eps);
}