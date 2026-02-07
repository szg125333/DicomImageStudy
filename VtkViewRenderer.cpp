#include "VtkViewRenderer.h"
#include <QVTKOpenGLNativeWidget.h>
#include <vtkImageViewer2.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleImage.h>
#include <vtkCallbackCommand.h>
#include <vtkImageData.h>

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

    // 一个统一的 callback
    m_vtkCmd = vtkSmartPointer<vtkCallbackCommand>::New();
    m_vtkCmd->SetCallback(VtkGenericCallback);
    m_vtkCmd->SetClientData(this);

    if (m_widget && m_widget->renderWindow()->GetInteractor()) {
        auto interactor = m_widget->renderWindow()->GetInteractor();
        interactor->AddObserver(vtkCommand::MouseWheelForwardEvent, m_vtkCmd);
        interactor->AddObserver(vtkCommand::MouseWheelBackwardEvent, m_vtkCmd);
        interactor->AddObserver(vtkCommand::LeftButtonPressEvent, m_vtkCmd);
        interactor->AddObserver(vtkCommand::RightButtonPressEvent, m_vtkCmd);
    }
}

VtkViewRenderer::~VtkViewRenderer() {
    if (m_widget && m_widget->renderWindow() && m_widget->renderWindow()->GetInteractor()) {
        auto interactor = m_widget->renderWindow()->GetInteractor();
        interactor->RemoveObservers(vtkCommand::MouseWheelForwardEvent);
        interactor->RemoveObservers(vtkCommand::MouseWheelBackwardEvent);
        interactor->RemoveObservers(vtkCommand::LeftButtonPressEvent);
        interactor->RemoveObservers(vtkCommand::RightButtonPressEvent);
    }
}

void VtkViewRenderer::SetInputData(vtkImageData* img) { m_viewer->SetInputData(img); }
void VtkViewRenderer::SetOrientation(SliceOrientation o) {
    if (o == SliceOrientation::XY) m_viewer->SetSliceOrientationToXY();
    else if (o == SliceOrientation::YZ) m_viewer->SetSliceOrientationToYZ();
    else m_viewer->SetSliceOrientationToXZ();
}
void VtkViewRenderer::SetSlice(int slice) { m_viewer->SetSlice(slice); }
int VtkViewRenderer::GetSlice() const { return m_viewer->GetSlice(); }
void VtkViewRenderer::Render() { m_viewer->Render(); }

void VtkViewRenderer::OnEvent(EventType type, std::function<void()> cb) {
    m_callbacks[type] = std::move(cb);
}

void VtkViewRenderer::VtkGenericCallback(vtkObject*, unsigned long eid, void* clientdata, void*) {
    auto self = static_cast<VtkViewRenderer*>(clientdata);
    if (!self) return;

    EventType type;
    switch (eid) {
    case vtkCommand::MouseWheelForwardEvent: type = EventType::WheelForward; break;
    case vtkCommand::MouseWheelBackwardEvent: type = EventType::WheelBackward; break;
    case vtkCommand::LeftButtonPressEvent: type = EventType::LeftClick; break;
    case vtkCommand::RightButtonPressEvent: type = EventType::RightClick; break;
    default: return;
    }

    auto it = self->m_callbacks.find(type);
    if (it != self->m_callbacks.end() && it->second) {
        it->second();
    }
}
