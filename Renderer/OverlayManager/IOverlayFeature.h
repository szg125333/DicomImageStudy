// IOverlayFeature.h
#pragma once
#include <array>
class vtkRenderer;
class vtkImageViewer2;

class IOverlayFeature {
public:
    virtual ~IOverlayFeature() = default;

    // 初始化：注入渲染器和视图上下文
    virtual void Initialize(vtkRenderer* renderer) = 0;

    // 可选：控制可见性（比如用户关闭测距）
    virtual void SetVisible(bool visible) {}

    // 可选：统一设置颜色（比如全局高亮色）
    virtual void SetColor(double r, double g, double b) {}

    // 清理资源（移除 actor、释放内存等）
    virtual void Shutdown() = 0;

    virtual void SetImageWorldBounds(const std::array<double, 6>& bounds) = 0;

    std::array<double, 6> m_imageWorldBounds = { 0, 0, 0, 0, 0, 0 };
    bool m_hasImageBounds = false;
};