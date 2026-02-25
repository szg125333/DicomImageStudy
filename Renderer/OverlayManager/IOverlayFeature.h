// IOverlayFeature.h
#pragma once

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
};