#pragma once
#include "IOverlayManager.h"
#include "Renderer/OverlayManager/CrosshairManager/ICrosshairManager.h"
#include "Renderer/OverlayManager/WindowLevelManager/IWindowLevelManager.h"
#include "Renderer/OverlayManager/DistanceMeasureManager/IDistanceMeasureManager.h"
#include <vtkSmartPointer.h>
#include <vtkActor.h>
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
    void SetVisible(bool visible) override;
    void SetColor(double r, double g, double b) override;
    void Shutdown() override;

    // 注册 feature（替代 SetXXXManager）
    void RegisterFeature(std::unique_ptr<IOverlayFeature> feature);

    // 实现基类的 GetFeatureImpl
    IOverlayFeature* GetFeatureImpl(const std::type_info& type) override {
        for (auto& feat : m_features) {
            if (typeid(*feat) == type) {
                return feat.get();
            }
        }
        return nullptr;
    }

private:
    vtkSmartPointer<vtkRenderer> m_overlayRenderer;
    vtkImageViewer2* m_viewer = nullptr; // 非拥有

    std::vector<std::unique_ptr<IOverlayFeature>> m_features;

    bool m_initialized = false;
    bool m_visible = true;
    double m_color[3] = { 0.0, 1.0, 0.0 };
};
