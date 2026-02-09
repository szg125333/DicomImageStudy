#pragma once
#include <array>
#include <vtkImageViewer2.h>

class vtkRenderer;

class IWindowLevelManager {
public:
    virtual ~IWindowLevelManager() = default;

    // 在 overlay renderer 上初始化文本 actor
    virtual void Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) = 0;

    // 设置窗宽窗位（单位与 viewer 一致）
    virtual void SetWindowLevel(double ww, double wl) = 0;

    // 控制可见性
    virtual void SetVisible(bool visible) = 0;

    // 从 renderer 中移除 actor 并释放资源
    virtual void Shutdown() = 0;
};
