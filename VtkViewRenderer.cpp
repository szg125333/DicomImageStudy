#include "VtkViewRenderer.h"
#include <QVTKOpenGLNativeWidget.h>
#include <vtkImageViewer2.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleImage.h>
#include <vtkCallbackCommand.h>
#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <QDebug>
#include "SimpleOverlayManager.h"  // 添加头文件

VtkViewRenderer::VtkViewRenderer(QVTKOpenGLNativeWidget* widget)
    : m_widget(widget)
{
    m_viewer = vtkSmartPointer<vtkImageViewer2>::New();
    m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    if (m_widget) {
        m_widget->setRenderWindow(m_renderWindow);
        m_viewer->SetRenderWindow(m_renderWindow);
        auto interactor = m_widget->renderWindow()->GetInteractor();
        if (interactor) {
            auto style = vtkSmartPointer<vtkInteractorStyleImage>::New();
            interactor->SetInteractorStyle(style);
        }
    }

    m_vtkCmd = vtkSmartPointer<vtkCallbackCommand>::New();
    m_vtkCmd->SetCallback(VtkGenericCallback);
    m_vtkCmd->SetClientData(this);

    if (m_widget && m_widget->renderWindow()->GetInteractor()) {
        auto interactor = m_widget->renderWindow()->GetInteractor();
        interactor->AddObserver(vtkCommand::MouseWheelForwardEvent, m_vtkCmd, 1.0f);
        interactor->AddObserver(vtkCommand::MouseWheelBackwardEvent, m_vtkCmd, 1.0f);
        interactor->AddObserver(vtkCommand::LeftButtonPressEvent, m_vtkCmd, 1.0f);
        interactor->AddObserver(vtkCommand::MouseMoveEvent, m_vtkCmd, 1.0f);
        interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, m_vtkCmd, 1.0f);
        interactor->AddObserver(vtkCommand::RightButtonPressEvent, m_vtkCmd, 1.0f);
    }

    m_renderWindow->SetNumberOfLayers(2);

    m_overlayRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_overlayRenderer->SetLayer(1);
    m_overlayRenderer->InteractiveOff();
    m_renderWindow->AddRenderer(m_overlayRenderer);
    m_overlayRenderer->SetActiveCamera(m_viewer->GetRenderer()->GetActiveCamera());
    m_overlayRenderer->ResetCameraClippingRange();

    // ===== 新增：创建 OverlayManager =====
    m_overlayManager = std::make_unique<SimpleOverlayManager>();
    m_overlayManager->Initialize(m_overlayRenderer, m_viewer.Get());

    // 🔴 设置 QTimer
    m_renderTimer.setSingleShot(true);  // 单次触发
    m_renderTimer.setInterval(0);       // 0ms 后触发（下一个事件循环）
    connect(&m_renderTimer, &QTimer::timeout, this, &VtkViewRenderer::PerformRender);
}

VtkViewRenderer::~VtkViewRenderer() {
    if (m_widget && m_widget->renderWindow() && m_widget->renderWindow()->GetInteractor()) {
        auto interactor = m_widget->renderWindow()->GetInteractor();
        interactor->RemoveObservers(vtkCommand::MouseWheelForwardEvent);
        interactor->RemoveObservers(vtkCommand::MouseWheelBackwardEvent);
        interactor->RemoveObservers(vtkCommand::LeftButtonPressEvent);
        interactor->RemoveObservers(vtkCommand::MouseMoveEvent);
        interactor->RemoveObservers(vtkCommand::LeftButtonReleaseEvent);
        interactor->RemoveObservers(vtkCommand::RightButtonPressEvent);
    }

    m_viewer->GetRenderer()->RemoveAllViewProps();
    m_viewer->GetRenderWindow()->Finalize();
}

void VtkViewRenderer::SetInputData(vtkImageData* img) { m_viewer->SetInputData(img); }
void VtkViewRenderer::SetOrientation(SliceOrientation o) {
    if (o == SliceOrientation::XY) m_viewer->SetSliceOrientationToXY();
    else if (o == SliceOrientation::YZ) m_viewer->SetSliceOrientationToYZ();
    else m_viewer->SetSliceOrientationToXZ();
}
void VtkViewRenderer::SetSlice(int slice) { m_viewer->SetSlice(slice); }
int VtkViewRenderer::GetSlice() const { return m_viewer->GetSlice(); }
//void VtkViewRenderer::Render() { m_viewer->Render(); }

void VtkViewRenderer::OnEvent(EventType type, std::function<void(void*)> cb) {
    m_callbacks[type] = std::move(cb);
}

void VtkViewRenderer::VtkGenericCallback(vtkObject* caller, unsigned long eid, void* clientdata, void* calldata) {
    auto self = static_cast<VtkViewRenderer*>(clientdata);
    if (!self) return;

    auto interactor = static_cast<vtkRenderWindowInteractor*>(caller);
    int pos[2];
    interactor->GetEventPosition(pos);

    EventType type;
    switch (eid) {
    case vtkCommand::MouseWheelForwardEvent:
        type = EventType::WheelForward;
        break;
    case vtkCommand::MouseWheelBackwardEvent:
        type = EventType::WheelBackward;
        break;
    case vtkCommand::LeftButtonPressEvent:
        type = EventType::LeftPress;
        break;
    case vtkCommand::MouseMoveEvent:
            type = EventType::LeftMove;
        break;
    case vtkCommand::LeftButtonReleaseEvent:
        type = EventType::LeftRelease;
        break;
    case vtkCommand::RightButtonPressEvent:
        type = EventType::RightClick;
        break;
    default:
        return;
    }

    auto it = self->m_callbacks.find(type);
    if (it != self->m_callbacks.end() && it->second) {
        // 注意：pos是局部数组，最好传递一个拷贝
        auto posCopy = std::make_shared<std::array<int, 2>>(std::array<int, 2>{pos[0], pos[1]});
        it->second(posCopy.get());

        vtkCallbackCommand* cb = static_cast<vtkCallbackCommand*>(self->m_vtkCmd);
        cb->SetAbortFlag(1);
    }
}

void VtkViewRenderer::RequestRender() {
    if (!m_renderTimer.isActive()) {
        m_renderTimer.start();  // 启动定时器
    }
}

void VtkViewRenderer::PerformRender() {
    m_renderPending = false;
    if (m_viewer) {
        m_viewer->Render();
    }
}