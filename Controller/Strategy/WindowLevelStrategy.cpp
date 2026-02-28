#include "WindowLevelStrategy.h"
#include "Interface/IViewController.h"
#include "Interface/IViewRenderer.h"
#include <vtkImageViewer2.h>
#include <vtkRenderer.h>
#include <vtkTextActor.h>
#include <vtkSmartPointer.h>
#include <vtkTextProperty.h>

WindowLevelStrategy::WindowLevelStrategy(IViewController* controller)
    : IInteractionStrategy(controller) {
}

void WindowLevelStrategy::HandleEvent(EventType type, int viewIndex, const EventData& data) {
    if (type == EventType::LeftPress) {
        //auto pos = static_cast<int*>(data);

        int pos[2];
        pos[0] = data.mousePosX;
        pos[1] = data.mousePosY;

        if (!pos) return;

        int dx = pos[0] - m_lastPos[0];
        int dy = pos[1] - m_lastPos[1];

        m_lastPos[0] = pos[0];
        m_lastPos[1] = pos[1];

        m_window += dx * m_sensitivityX;
        m_level += dy * m_sensitivityY;

        updateWindowLevel(viewIndex);
    }
}

void WindowLevelStrategy::updateWindowLevel(int viewIndex) {
    auto renderer = m_controller->GetRenderer(viewIndex);
    if (!renderer) return;

    // 获取 VTK viewer
    auto viewer = renderer->GetViewer();
    if (!viewer) return;

    viewer->SetColorWindow(m_window);
    viewer->SetColorLevel(m_level);

    // 更新 overlay 显示
    static vtkSmartPointer<vtkTextActor> textActor = vtkSmartPointer<vtkTextActor>::New();
    textActor->SetInput(QString("WW:%1 WL:%2").arg(m_window).arg(m_level).toStdString().c_str());
    textActor->GetTextProperty()->SetColor(1.0, 1.0, 0.0);
    textActor->SetDisplayPosition(10, 10);

    viewer->GetRenderer()->AddActor2D(textActor);
    viewer->Render();
}
