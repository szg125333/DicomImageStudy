#include "VtkViewRenderer.h"
#include "Renderer/OverlayManager/IOverlayManager.h"

#include <QVTKOpenGLNativeWidget.h>
#include <vtkImageViewer2.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleImage.h>
#include <vtkCallbackCommand.h>
#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkPropPicker.h>
#include <vtkPointPicker.h>

#include <limits>

// ============================================================
//  构造 / 析构
// ============================================================

VtkViewRenderer::VtkViewRenderer(QVTKOpenGLNativeWidget* widget)
    : QObject(nullptr)
    , m_widget(widget)
{
    // ---------- VTK 图像查看器初始化 ----------
    m_viewer = vtkSmartPointer<vtkImageViewer2>::New();
    m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();

    if (m_widget) {
        m_widget->setRenderWindow(m_renderWindow);
        m_viewer->SetRenderWindow(m_renderWindow);

        // 使用图像交互风格，禁用默认的三维旋转
        auto interactor = m_widget->renderWindow()->GetInteractor();
        if (interactor) {
            auto style = vtkSmartPointer<vtkInteractorStyleImage>::New();
            interactor->SetInteractorStyle(style);
        }
    }

    // ---------- VTK 事件监听回调 ----------
    m_vtkCmd = vtkSmartPointer<vtkCallbackCommand>::New();
    m_vtkCmd->SetCallback(VtkEventCallback);
    m_vtkCmd->SetClientData(this);

    if (m_widget) {
        auto interactor = m_widget->renderWindow()->GetInteractor();
        if (interactor) {
            // 优先级 1.0f，确保在 VTK 默认处理之前截获事件
            interactor->AddObserver(vtkCommand::MouseWheelForwardEvent, m_vtkCmd, 1.0f);
            interactor->AddObserver(vtkCommand::MouseWheelBackwardEvent, m_vtkCmd, 1.0f);
            interactor->AddObserver(vtkCommand::LeftButtonPressEvent, m_vtkCmd, 1.0f);
            interactor->AddObserver(vtkCommand::MouseMoveEvent, m_vtkCmd, 1.0f);
            interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, m_vtkCmd, 1.0f);
            interactor->AddObserver(vtkCommand::RightButtonPressEvent, m_vtkCmd, 1.0f);
            interactor->AddObserver(vtkCommand::KeyPressEvent, m_vtkCmd, 1.0f);
            interactor->AddObserver(vtkCommand::KeyReleaseEvent, m_vtkCmd, 1.0f);
        }
    }

    // ---------- Overlay 渲染器（第二层）初始化 ----------
    // 双层渲染：Layer 0 = 图像，Layer 1 = Overlay 标注
    m_renderWindow->SetNumberOfLayers(2);
    m_viewer->GetRenderer()->SetLayer(0);

    m_overlayRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_overlayRenderer->SetLayer(1);
    m_overlayRenderer->InteractiveOff();       // 不响应用户交互
    m_overlayRenderer->EraseOff();             // 不清空背景，透明叠加
    m_overlayRenderer->SetBackground(0, 0, 0);
    m_overlayRenderer->SetGradientBackground(false);

    // 与图像渲染器共享同一个相机，保证坐标对齐
    m_overlayRenderer->SetActiveCamera(m_viewer->GetRenderer()->GetActiveCamera());

    double viewport[4];
    m_viewer->GetRenderer()->GetViewport(viewport);
    m_overlayRenderer->SetViewport(viewport);

    m_renderWindow->AddRenderer(m_overlayRenderer);

    // ---------- 延迟渲染计时器（合并高频请求）----------
    m_renderTimer.setSingleShot(true);
    m_renderTimer.setInterval(16);  // ~60 fps 上限
    connect(&m_renderTimer, &QTimer::timeout,
        this, &VtkViewRenderer::OnRenderTimerTimeout);

    // ---------- 状态变化信号 → Overlay 刷新 ----------
    connect(this, &VtkViewRenderer::viewStateChanged,
        this, &VtkViewRenderer::OnViewStateChanged);
}

VtkViewRenderer::~VtkViewRenderer()
{
    // 注销所有 VTK 事件监听，防止回调访问已销毁对象
    if (m_widget && m_widget->renderWindow()) {
        auto interactor = m_widget->renderWindow()->GetInteractor();
        if (interactor) {
            interactor->RemoveObservers(vtkCommand::MouseWheelForwardEvent);
            interactor->RemoveObservers(vtkCommand::MouseWheelBackwardEvent);
            interactor->RemoveObservers(vtkCommand::LeftButtonPressEvent);
            interactor->RemoveObservers(vtkCommand::MouseMoveEvent);
            interactor->RemoveObservers(vtkCommand::LeftButtonReleaseEvent);
            interactor->RemoveObservers(vtkCommand::RightButtonPressEvent);
            interactor->RemoveObservers(vtkCommand::KeyPressEvent);
            interactor->RemoveObservers(vtkCommand::KeyReleaseEvent);
        }
    }

    if (m_viewer) {
        m_viewer->GetRenderer()->RemoveAllViewProps();
        m_viewer->GetRenderWindow()->Finalize();
    }
}

// ============================================================
//  图像数据
// ============================================================

void VtkViewRenderer::SetInputData(vtkImageData* image)
{
    if (m_viewer) {
        m_viewer->SetInputData(image);
    }
}

// ============================================================
//  切片控制
// ============================================================

void VtkViewRenderer::SetOrientation(ViewType viewType)
{
    if (!m_viewer) return;

    switch (viewType) {
    case ViewType::Axial:
        m_viewer->SetSliceOrientationToXY();
        break;
    case ViewType::Sagittal:
        m_viewer->SetSliceOrientationToYZ();
        break;
    case ViewType::Coronal:
        m_viewer->SetSliceOrientationToXZ();
        break;
    default:
        return;
    }
    m_currentViewType = viewType;
}

void VtkViewRenderer::SetSlice(int slice)
{
    if (m_viewer) {
        m_currentSlice = slice;
        m_viewer->SetSlice(slice);
    }
}

void VtkViewRenderer::SetMaxSlice(int maxSlice)
{
    m_maxSlices = maxSlice;
}

int VtkViewRenderer::GetSlice() const
{
    return m_viewer ? m_viewer->GetSlice() : 0;
}

// ============================================================
//  窗宽窗位
// ============================================================

void VtkViewRenderer::SetColorWindow(double window)
{
    m_windowWidth = window;
    if (m_viewer) {
        m_viewer->SetColorWindow(window);
    }
}

void VtkViewRenderer::SetColorLevel(double level)
{
    m_windowLevel = level;
    if (m_viewer) {
        m_viewer->SetColorLevel(level);
    }
}

// ============================================================
//  坐标拾取
// ============================================================

std::array<double, 3> VtkViewRenderer::PickWorldPosition(int screenX, int screenY)
{
    constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

    if (!m_viewer) return { kNaN, kNaN, kNaN };

    auto* renderer = m_viewer->GetRenderer();
    if (!renderer) return { kNaN, kNaN, kNaN };

    // 优先尝试拾取 Overlay Actor（测量端点等）
    auto propPicker = vtkSmartPointer<vtkPropPicker>::New();
    if (propPicker->PickProp(screenX, screenY, renderer)) {
        double p[3];
        propPicker->GetPickPosition(p);
        return { p[0], p[1], p[2] };
    }

    // 回退到图像点拾取
    auto pointPicker = vtkSmartPointer<vtkPointPicker>::New();
    if (pointPicker->Pick(screenX, screenY, 0, renderer)) {
        double p[3];
        pointPicker->GetPickPosition(p);
        return { p[0], p[1], p[2] };
    }

    return { kNaN, kNaN, kNaN };
}

void VtkViewRenderer::SetCurrentClickWorldPos(std::array<double, 3> worldPos)
{
    m_currentClickWorldPos = worldPos;
}

// ============================================================
//  渲染控制
// ============================================================

void VtkViewRenderer::RequestRender()
{
    if (!m_renderTimer.isActive()) {
        // 每次请求渲染时同步更新 Overlay 信息
        UpdateBasicInfoActor();
        m_renderTimer.start();
    }
}

void VtkViewRenderer::OnRenderTimerTimeout()
{
    if (m_viewer) {
        m_viewer->Render();
    }
}

// ============================================================
//  Overlay 信息层
// ============================================================

void VtkViewRenderer::UpdateBasicInfoActor()
{
    RenderViewState state;
    state.viewType = GetCurrentViewType();
    state.windowWidth = GetColorWindow();
    state.windowLevel = GetColorLevel();
    state.sliceIndex = GetCurrentSlice();
    state.worldPos = GetCurrentClickWorldPos();
    emit viewStateChanged(state);
}

void VtkViewRenderer::OnViewStateChanged(const RenderViewState& state)
{
    if (m_overlayManager) {
        m_overlayManager->UpdateBasicInfoActor(state);
    }
}

// ============================================================
//  Overlay 管理器
// ============================================================

void VtkViewRenderer::SetOverlayManager(std::unique_ptr<IOverlayManager> manager)
{
    m_overlayManager = std::move(manager);

    // 立即刷新一次 Overlay 信息（初始状态同步）
    RenderViewState state;
    state.viewType = m_currentViewType;
    state.windowWidth = m_windowWidth;
    state.windowLevel = m_windowLevel;
    state.sliceIndex = m_currentSlice;
    m_overlayManager->UpdateBasicInfoActor(state);
}

// ============================================================
//  事件注册
// ============================================================

void VtkViewRenderer::OnEvent(EventType type,
    std::function<void(const EventData&)> cb)
{
    m_callbacks[type] = std::move(cb);
}

// ============================================================
//  VTK 静态回调
// ============================================================

void VtkViewRenderer::VtkEventCallback(vtkObject* caller,
    unsigned long eventId,
    void* clientData,
    void* /*callData*/)
{
    auto* self = static_cast<VtkViewRenderer*>(clientData);
    if (!self) return;

    auto* interactor = static_cast<vtkRenderWindowInteractor*>(caller);
    if (!interactor) return;

    // ---------- 填充事件数据 ----------
    int pos[2];
    interactor->GetEventPosition(pos);

    EventData eventData;
    eventData.mousePosX = pos[0];
    eventData.mousePosY = pos[1];
    eventData.ctrlPressed = interactor->GetControlKey() != 0;
    eventData.shiftPressed = interactor->GetShiftKey() != 0;
    eventData.altPressed = interactor->GetAltKey() != 0;

    // ---------- VTK 事件 → 应用层事件类型 ----------
    EventType type;
    switch (eventId) {
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
        const char* keySym = interactor->GetKeySym();
        if (keySym) eventData.keySym = keySym;
        type = EventType::KeyPress;
        break;
    }
    case vtkCommand::KeyReleaseEvent: {
        const char* keySym = interactor->GetKeySym();
        if (keySym) eventData.keySym = keySym;
        type = EventType::KeyRelease;
        break;
    }
    default:
        return;
    }

    // ---------- 分发到已注册的回调 ----------
    auto it = self->m_callbacks.find(type);
    if (it != self->m_callbacks.end() && it->second) {
        it->second(eventData);

        // 阻止事件继续传递给 VTK 默认交互器
        static_cast<vtkCallbackCommand*>(self->m_vtkCmd.Get())->SetAbortFlag(1);
    }
}
