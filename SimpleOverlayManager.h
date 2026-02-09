#pragma once
#include "IOverlayManager.h"
#include "ICrosshairManager.h"
#include "IWindowLevelManager.h"
#include <vtkSmartPointer.h>
#include <array>
#include <memory>

class vtkRenderer;
class vtkImageViewer2;

// SimpleOverlayManager 将内部组合多个子 manager。
// 子 manager 通过接口注入或由 SimpleOverlayManager 创建默认实现。
class SimpleOverlayManager : public IOverlayManager {
public:
    SimpleOverlayManager();
    ~SimpleOverlayManager() override;

    // IOverlayManager
    void Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) override;
    void UpdateCrosshair(const std::array<double, 3>& worldPoint,
        ViewType view,
        const std::array<double, 3>& worldMin,
        const std::array<double, 3>& worldMax) override;
    void SetWindowLevel(double ww, double wl) override;
    void SetVisible(bool visible) override;
    void SetColor(double r, double g, double b) override;
    void Shutdown() override;

    // 可选：注入自定义子 manager（在 Initialize 之前调用）
    void SetCrosshairManager(std::unique_ptr<ICrosshairManager> mgr);
    void SetWindowLevelManager(std::unique_ptr<IWindowLevelManager> mgr);

    void UpdateCrosshair(std::array<double, 3> worldPoint, ViewType view, const double worldMin[3], const double worldMax[3]);
    void UpdateCrosshairInAllViews(const std::array<double, 3>& worldPoint,
        const std::array<double, 3>& worldMin,
        const std::array<double, 3>& worldMax);
private:
    void EnsureDefaults(); // 创建默认子 manager（如果未注入）

private:
    vtkSmartPointer<vtkRenderer> m_overlayRenderer;
    vtkImageViewer2* m_viewer = nullptr; // 非拥有

    std::unique_ptr<ICrosshairManager> m_crosshairManager;
    std::unique_ptr<IWindowLevelManager> m_windowLevelManager;

    bool m_initialized = false;
    bool m_visible = true;
    double m_color[3] = { 0.0, 1.0, 0.0 };
};
