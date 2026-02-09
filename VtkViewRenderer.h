#pragma once
#include "IViewRenderer.h"
#include "InteractionMode.h"
#include <QPointer>
#include <QTimer>
#include <unordered_map>
#include <functional>

#include <vtkSmartPointer.h>
#include <vtkObject.h>

class QVTKOpenGLNativeWidget;
class vtkImageViewer2;
class vtkGenericOpenGLRenderWindow;
class vtkCallbackCommand;
class vtkImageData;
class IOverlayManager;

class VtkViewRenderer : public QObject,  public IViewRenderer {
    Q_OBJECT
public:
    VtkViewRenderer(QVTKOpenGLNativeWidget* widget);
    ~VtkViewRenderer() override;

    void SetInputData(vtkImageData* img) override;
    void SetOrientation(SliceOrientation o) override;
    void SetSlice(int slice) override;
    int GetSlice() const override;
    //void Render() override;
    void RequestRender() override;

    // 统一注册接口
    void OnEvent(EventType type, std::function<void(void*)> cb);
    
    vtkSmartPointer<vtkImageViewer2> GetViewer() override { return m_viewer; }
    vtkSmartPointer<vtkRenderer> GetOverlayRenderer() { return m_overlayRenderer; }

    IOverlayManager* GetOverlayManager() override { return m_overlayManager.get(); }

private slots:
    void PerformRender();  // ← 新增

private:
    QPointer<QVTKOpenGLNativeWidget> m_widget;
    vtkSmartPointer<vtkImageViewer2> m_viewer;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer>  m_overlayRenderer;

    std::unordered_map<EventType, std::function<void(void*)>> m_callbacks;
    vtkSmartPointer<vtkCallbackCommand> m_vtkCmd;
    static void VtkGenericCallback(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

    //bool m_dragging = false; // 拖动状态标志

    // ===== 新增：管理所有 overlay 渲染对象 =====
    std::unique_ptr<IOverlayManager> m_overlayManager;

    bool m_renderPending = false;  // ← 新增

    QTimer m_renderTimer;  // 🔴 使用 QTimer
};
