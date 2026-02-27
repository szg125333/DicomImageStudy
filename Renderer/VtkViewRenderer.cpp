#include "VtkViewRenderer.h"
#include "Renderer/OverlayManager/SimpleOverlayManager.h"
#include <QVTKOpenGLNativeWidget.h>
#include <vtkImageViewer2.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleImage.h>
#include <vtkCallbackCommand.h>
#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkCellPicker.h>
#include <vtkPropPicker.h>
#include <vtkPointPicker.h>

// ==================== 构造与析构 ====================

VtkViewRenderer::VtkViewRenderer(QVTKOpenGLNativeWidget* widget)
    : QObject(nullptr), m_widget(widget)
{
    // ===== VTK 图像查看器初始化 =====
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

    // ===== VTK 事件回调设置 =====
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
        interactor->AddObserver(vtkCommand::KeyPressEvent, m_vtkCmd, 1.0f);
        interactor->AddObserver(vtkCommand::KeyReleaseEvent, m_vtkCmd, 1.0f);
    }

    // ===== Overlay 渲染器初始化 =====
    m_renderWindow->SetNumberOfLayers(2);

    m_overlayRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_overlayRenderer->SetLayer(1);
    m_overlayRenderer->InteractiveOff();
    m_renderWindow->AddRenderer(m_overlayRenderer);
    m_overlayRenderer->SetActiveCamera(m_viewer->GetRenderer()->GetActiveCamera());
    m_overlayRenderer->ResetCameraClippingRange();

    // ===== 延迟渲染计时器设置 =====
    m_renderTimer.setSingleShot(true);
    m_renderTimer.setInterval(0);
    connect(&m_renderTimer, &QTimer::timeout, this, &VtkViewRenderer::PerformRender);
}

VtkViewRenderer::~VtkViewRenderer() {
    // ===== 清理 VTK 事件观察器 =====
    if (m_widget && m_widget->renderWindow() && m_widget->renderWindow()->GetInteractor()) {
        auto interactor = m_widget->renderWindow()->GetInteractor();
        interactor->RemoveObservers(vtkCommand::MouseWheelForwardEvent);
        interactor->RemoveObservers(vtkCommand::MouseWheelBackwardEvent);
        interactor->RemoveObservers(vtkCommand::LeftButtonPressEvent);
        interactor->RemoveObservers(vtkCommand::MouseMoveEvent);
        interactor->RemoveObservers(vtkCommand::LeftButtonReleaseEvent);
        interactor->RemoveObservers(vtkCommand::RightButtonPressEvent);
        interactor->RemoveObservers(vtkCommand::KeyPressEvent);
        interactor->RemoveObservers(vtkCommand::KeyReleaseEvent);
    }

    // ===== 清理 VTK 资源 =====
    if (m_viewer) {
        m_viewer->GetRenderer()->RemoveAllViewProps();
        m_viewer->GetRenderWindow()->Finalize();
    }
}

// ==================== 公开方法 ====================

void VtkViewRenderer::SetInputData(vtkImageData* img) {
    if (m_viewer) {
        m_viewer->SetInputData(img);
    }
}

void VtkViewRenderer::SetOrientation(SliceOrientation o) {
    if (m_viewer) {
        if (o == SliceOrientation::XY) {
            m_viewer->SetSliceOrientationToXY();
        }
        else if (o == SliceOrientation::YZ) {
            m_viewer->SetSliceOrientationToYZ();
        }
        else {
            m_viewer->SetSliceOrientationToXZ();
        }
    }
}

void VtkViewRenderer::SetSlice(int slice) {
    if (m_viewer) {
        m_viewer->SetSlice(slice);
    }
}

int VtkViewRenderer::GetSlice() const {
    if (m_viewer) {
        return m_viewer->GetSlice();
    }
    return 0;
}

void VtkViewRenderer::OnEvent(EventType type, std::function<void(const EventData&)> cb) {
    m_callbacks[type] = std::move(cb);
}

void VtkViewRenderer::SetOverlayManager(std::unique_ptr<IOverlayManager> manager)
{
    m_overlayManager = std::move(manager);
}

std::array<double, 3> VtkViewRenderer::PickWorldPosition(int screenX, int screenY)
{
    vtkSmartPointer<vtkPropPicker> picker = vtkSmartPointer<vtkPropPicker>::New();

    // 优先拾取 overlay 中的 actor（如测距端点）
    if (picker->PickProp(screenX, screenY, m_viewer->GetRenderer())) {
        double p[3];
        picker->GetPickPosition(p);
        return { p[0], p[1], p[2] };
    }

    // 可选：如果没拾取到 overlay，再尝试拾取背景图像（用 vtkPointPicker）
    vtkSmartPointer<vtkPointPicker> bgPicker = vtkSmartPointer<vtkPointPicker>::New();
    if (bgPicker->Pick(screenX, screenY, 0, m_viewer->GetRenderer())) {
        double p[3];
        bgPicker->GetPickPosition(p);
        return { p[0], p[1], p[2] };
    }

    // 真的没拾取到：返回 NaN（或抛异常 / optional）
    return {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN()
    };
}


void VtkViewRenderer::RequestRender() {
    if (!m_renderTimer.isActive()) {
        m_renderTimer.start();
    }
}

// ==================== 私有方法 ====================

void VtkViewRenderer::PerformRender() {
    if (m_viewer) {
        m_viewer->Render();
    }
}

void VtkViewRenderer::VtkGenericCallback(vtkObject* caller, unsigned long eid,
    void* clientdata, void* calldata) {
    // ===== 参数转换 =====
    auto self = static_cast<VtkViewRenderer*>(clientdata);
    if (!self) return;

    auto interactor = static_cast<vtkRenderWindowInteractor*>(caller);
    int pos[2];
    interactor->GetEventPosition(pos);

    EventData eventData;
	eventData.mousePosX = pos[0];
	eventData.mousePosY = pos[1];
    eventData.ctrlPressed = interactor->GetControlKey() != 0;
    eventData.shiftPressed = interactor->GetShiftKey() != 0;
    eventData.altPressed = interactor->GetAltKey() != 0;

    // ===== VTK 事件转换为应用层事件 =====
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
            type = EventType::RightPress;
            break;
        case vtkCommand::KeyPressEvent: {
            char* keySymPress = interactor->GetKeySym();
            if (keySymPress) {
                eventData.keySym = keySymPress;
            }
            type = EventType::KeyPress;
            break;
        }
        case vtkCommand::KeyReleaseEvent: {
            char* keySymRelease = interactor->GetKeySym();
            if (keySymRelease) {
                eventData.keySym = keySymRelease;
            }
            type = EventType::KeyRelease;
            break;
        }
        default:
            return;
    }


    // ===== 调用已注册的回调函数 =====
    auto it = self->m_callbacks.find(type);
    if (it != self->m_callbacks.end() && it->second) {
        auto posCopy = std::make_shared<std::array<int, 2>>(
            std::array<int, 2>{pos[0], pos[1]}
        );
        //it->second(posCopy.get());
        it->second(eventData);

        vtkCallbackCommand* cmd = static_cast<vtkCallbackCommand*>(self->m_vtkCmd);
        cmd->SetAbortFlag(1);
    }
}
