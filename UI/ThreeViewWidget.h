#pragma once

#include <QWidget>
#include <QPointer>
#include <array>
#include <memory>
#include <vtkImageData.h>
#include "Common/ViewTypes.h"

class QVTKOpenGLNativeWidget;
class VtkViewRenderer;
class ThreeViewController;

class ThreeViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ThreeViewWidget(QWidget* parent = nullptr, ThreeViewController* controller = nullptr);
    ~ThreeViewWidget() override;

    // 将图像数据传入控制器（由控制器分发到各 renderer）
    void SetImageData(vtkImageData* image);

    // 直接请求设置某视图切片（转发到 controller）
    void RequestSetSlice(ViewType view, int slice);

signals:
    // 对外暴露切片变化信号（由 controller 转发）
    void sliceChanged(int viewIndex, int slice);

private:
    void setupUi();
    void setupRenderersAndController();

private:
    QPointer<QVTKOpenGLNativeWidget> m_widgets[3];

    // UI 层拥有 renderer 的实例（封装 VTK 细节）
    std::unique_ptr<VtkViewRenderer> m_renderers[3];

    // Controller 可以由外部注入，也可以由 widget 创建并拥有
    ThreeViewController* m_controller = nullptr;
    bool m_controllerOwned = false;
};

