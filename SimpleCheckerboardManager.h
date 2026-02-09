#pragma once
#include "ICheckerboardManager.h"
#include <vtkSmartPointer.h>

class vtkRenderer;
class vtkImageViewer2;

/// @brief 棋盘格对比模式管理器实现
class SimpleCheckerboardManager : public ICheckerboardManager {
public:
    SimpleCheckerboardManager();
    ~SimpleCheckerboardManager() override;

    void Initialize(vtkRenderer* overlayRenderer, vtkImageViewer2* viewer) override;
    void SetCheckerboardMode(bool enabled, double ratio = 0.5) override;
    void SetVisible(bool visible) override;
    void Shutdown() override;

private:
    vtkSmartPointer<vtkRenderer> m_overlayRenderer;
    vtkImageViewer2* m_viewer = nullptr;
    bool m_initialized = false;
    bool m_visible = true;
};