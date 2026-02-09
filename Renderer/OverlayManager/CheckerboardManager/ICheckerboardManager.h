#pragma once

class vtkRenderer;
class vtkImageViewer2;

/// @brief 棋盘格对比模式管理器接口
class ICheckerboardManager {
public:
    virtual ~ICheckerboardManager() = default;

    /// @brief 初始化管理器
    virtual void Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) = 0;

    /// @brief 设置棋盘格模式
    /// @param enabled 是否启用
    /// @param ratio 分割比例（0.5 表示 50/50）
    virtual void SetCheckerboardMode(bool enabled, double ratio = 0.5) = 0;

    /// @brief 设置可见性
    virtual void SetVisible(bool visible) = 0;

    /// @brief 清理资源
    virtual void Shutdown() = 0;
};