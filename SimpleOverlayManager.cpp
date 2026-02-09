#include "SimpleOverlayManager.h"
#include "SimpleCrosshairManager.h"
#include "SimpleWindowLevelManager.h"
#include <vtkRenderer.h>
#include <vtkImageViewer2.h>
#include <vtkImageData.h>
#include <QString>

SimpleOverlayManager::SimpleOverlayManager() = default;

SimpleOverlayManager::~SimpleOverlayManager() {
    Shutdown();
}

void SimpleOverlayManager::EnsureDefaults() {
    if (!m_crosshairManager) {
        m_crosshairManager = std::make_unique<SimpleCrosshairManager>();
    }
    if (!m_windowLevelManager) {
        m_windowLevelManager = std::make_unique<SimpleWindowLevelManager>();
    }
}

void SimpleOverlayManager::Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) {
    if (m_initialized) return;
    if (!overlayRenderer) return;

    m_overlayRenderer = overlayRenderer;
    m_viewer = viewer;

    EnsureDefaults();

    // 初始化子模块（子模块内部会把 actor 添加到 overlayRenderer）
    if (m_crosshairManager) m_crosshairManager->Initialize(overlayRenderer);
    if (m_windowLevelManager) m_windowLevelManager->Initialize(overlayRenderer, viewer);

    //// 如果已经有 image，传给子模块
    //if (m_image) {
    //    if (m_windowLevelManager) m_windowLevelManager->SetImageData(m_image);
    //}

    // 应用初始样式/可见性
    SetVisible(m_visible);
    SetColor(m_color[0], m_color[1], m_color[2]);

    m_initialized = true;
}

void SimpleOverlayManager::SetImageData(vtkImageData* image) {
    if (!image) {
        m_image = nullptr;
    }
    else {
        m_image = image; // vtkSmartPointer 接收引用计数
    }
    //if (m_windowLevelManager) m_windowLevelManager->SetImageData(m_image);
}

void SimpleOverlayManager::UpdateCrosshair(const std::array<double, 3>& worldPoint,
    ViewType view,
    const std::array<double, 3>& worldMin,
    const std::array<double, 3>& worldMax) {
    if (!m_initialized) {
        // 允许在未初始化时缓存或忽略；这里直接忽略以简化逻辑
        return;
    }
    if (m_crosshairManager) {
        // 子 manager 接口可能接受 std::array 或 C 数组，适配调用
        m_crosshairManager->UpdateCrosshair(worldPoint, view, worldMin.data(), worldMax.data());
    }
}

void SimpleOverlayManager::SetWindowLevel(double ww, double wl) {
    if (m_windowLevelManager) {
        m_windowLevelManager->SetWindowLevel(ww, wl);
    }
    //// 同步到 viewer（若需要）
    //if (m_viewer) {
    //    m_viewer->SetColorWindow(ww);
    //    m_viewer->SetColorLevel(wl);
    //}
}

void SimpleOverlayManager::SetVisible(bool visible) {
    m_visible = visible;
    if (!m_initialized) return;
    if (m_crosshairManager) m_crosshairManager->SetVisible(visible);
    if (m_windowLevelManager) m_windowLevelManager->SetVisible(visible);
}

void SimpleOverlayManager::SetColor(double r, double g, double b) {
    m_color[0] = r; m_color[1] = g; m_color[2] = b;
    if (!m_initialized) return;
    if (m_crosshairManager) m_crosshairManager->SetColor(r, g, b);
    if (m_windowLevelManager) {
        // window level manager 可能不使用颜色，但如果需要可以提供接口
    }
}

void SimpleOverlayManager::Shutdown() {
    if (!m_initialized) return;

    // 子模块 Shutdown（在渲染线程/主线程调用）
    if (m_crosshairManager) {
        m_crosshairManager->Shutdown();
        m_crosshairManager.reset();
    }
    if (m_windowLevelManager) {
        m_windowLevelManager->Shutdown();
        m_windowLevelManager.reset();
    }

    m_image = nullptr;
    m_viewer = nullptr;
    m_overlayRenderer = nullptr;
    m_initialized = false;
}

void SimpleOverlayManager::SetCrosshairManager(std::unique_ptr<ICrosshairManager> mgr) {
    m_crosshairManager = std::move(mgr);
}

void SimpleOverlayManager::SetWindowLevelManager(std::unique_ptr<IWindowLevelManager> mgr) {
    m_windowLevelManager = std::move(mgr);
}
