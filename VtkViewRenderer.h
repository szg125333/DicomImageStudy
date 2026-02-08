#pragma once
#include "IViewRenderer.h"
#include "InteractionMode.h"
#include <QPointer>
#include <unordered_map>
#include <functional>

#include <vtkSmartPointer.h>
#include <vtkObject.h>

class QVTKOpenGLNativeWidget;
class vtkImageViewer2;
class vtkGenericOpenGLRenderWindow;
class vtkCallbackCommand;
class vtkImageData;

class VtkViewRenderer : public IViewRenderer {
public:
    VtkViewRenderer(QVTKOpenGLNativeWidget* widget);
    ~VtkViewRenderer() override;

    void SetInputData(vtkImageData* img) override;
    void SetOrientation(SliceOrientation o) override;
    void SetSlice(int slice) override;
    int GetSlice() const override;
    void Render() override;

    // 统一注册接口
    void OnEvent(EventType type, std::function<void(void*)> cb);
    
    vtkSmartPointer<vtkImageViewer2> GetViewer() override { return m_viewer; }
    vtkSmartPointer<vtkRenderer> GetOverlayRenderer() { return m_overlayRenderer; }

private:
    QPointer<QVTKOpenGLNativeWidget> m_widget;
    vtkSmartPointer<vtkImageViewer2> m_viewer;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer>  m_overlayRenderer;

    std::unordered_map<EventType, std::function<void(void*)>> m_callbacks;
    vtkSmartPointer<vtkCallbackCommand> m_vtkCmd;
    static void VtkGenericCallback(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

    bool m_dragging = false; // 拖动状态标志
};
