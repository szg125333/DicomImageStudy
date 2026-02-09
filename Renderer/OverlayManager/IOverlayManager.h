#pragma once
#include <array>
#include <memory>
#include <vector>

class vtkRenderer;
class vtkImageData;
class vtkImageViewer2;
enum class ViewType;

class IOverlayManager {
public:
    virtual ~IOverlayManager() = default;

    // 初始化 overlay，传入 overlay renderer 与 viewer（非拥有）
    virtual void Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) = 0;

    // 更新十字线（controller 传入 worldPoint 与图像 world 范围）
    virtual void UpdateCrosshair(const std::array<double, 3>& worldPoint,
        ViewType view,
        const std::array<double, 3>& worldMin,
        const std::array<double, 3>& worldMax) = 0;

    // 设置窗宽窗位（会同时应用到 viewer 与 overlay 文本）
    virtual void SetWindowLevel(double ww, double wl) = 0;

    // 控制 overlay 可见性（整体）
    virtual void SetVisible(bool visible) = 0;

    // 设置 overlay 全局颜色/样式（可由子模块选择性使用）
    virtual void SetColor(double r, double g, double b) = 0;

    // 清理资源（必须幂等，且在渲染线程或主线程调用）
    virtual void Shutdown() = 0;
    virtual void UpdateCrosshairInAllViews(const std::array<double, 3>& worldPoint,
        const std::array<double, 3>& worldMin,
        const std::array<double, 3>& worldMax) = 0;
};
