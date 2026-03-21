#include "ThreeViewController.h"
#include "Interface/IViewRenderer.h"
#include "Renderer/OverlayManager/IOverlayManager.h"
#include "Renderer/OverlayManager/OverlayFactory.h"
#include "Controller/Strategy/InteractionStrategyFactory.h"
#include "Controller/Strategy/IInteractionStrategy.h"

#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkRenderWindow.h>
#include <vtkImageViewer2.h>

#include <cmath>

// ============================================================
//  构造 / 析构
// ============================================================

ThreeViewController::ThreeViewController(QObject* parent)
    : QObject(parent)
{
    m_renderers.fill(nullptr);

    // 预先创建所有策略（策略生命周期与控制器一致）
    m_strategies = InteractionStrategyFactory::CreateStrategies(this);

    // 初始模式设为普通浏览
    m_currentMode = InteractionMode::Normal;
}

ThreeViewController::~ThreeViewController() = default;

// ============================================================
//  初始化
// ============================================================

void ThreeViewController::SetRenderers(std::array<IViewRenderer*, 3> renderers)
{
    m_renderers = renderers;
}

void ThreeViewController::SetImageData(vtkImageData* image)
{
    if (!image) return;
    m_image = image;

    // 将图像数据绑定到三个渲染器
    for (auto* r : m_renderers) {
        if (r) r->SetInputData(image);
    }

    ComputeSliceRanges();

    // 初始切片定位到图像中心
    int dims[3];
    m_image->GetDimensions(dims);
    SetSliceInternal(ViewType::Axial, dims[2] / 2);
    SetSliceInternal(ViewType::Sagittal, dims[0] / 2);
    SetSliceInternal(ViewType::Coronal, dims[1] / 2);

    // 根据图像灰度范围初始化窗宽窗位
    double range[2];
    m_image->GetScalarRange(range);
    m_windowWidth = range[1] - range[0];
    m_windowLevel = (range[0] + range[1]) / 2.0;

    for (auto* r : m_renderers) {
        if (r) {
            r->SetColorWindow(m_windowWidth);
            r->SetColorLevel(m_windowLevel);
        }
    }

    // 使用正交投影（医学影像切片标准显示方式）
    for (auto* r : m_renderers) {
        if (!r) continue;
        auto viewer = r->GetViewer();
        if (viewer && viewer->GetRenderer()) {
            viewer->GetRenderer()->GetActiveCamera()->SetParallelProjection(true);
            viewer->GetRenderer()->ResetCamera();
        }
    }

    // 注册事件回调（图像就绪后才有意义）
    RegisterEventCallbacks();
}

// ============================================================
//  IViewController 接口实现
// ============================================================

IViewRenderer* ThreeViewController::GetRenderer(int viewIndex)
{
    if (viewIndex < 0 || viewIndex >= 3) return nullptr;
    return m_renderers[viewIndex];
}

const vtkImageData* ThreeViewController::GetImage() const
{
    return m_image.Get();
}

void ThreeViewController::ChangeSlice(int viewIndex, int delta)
{
    if (viewIndex < 0 || viewIndex >= 3) return;

    const ViewType view = static_cast<ViewType>(viewIndex);
    const int idx = viewIndex;

    int newSlice = m_renderers[idx] ? m_renderers[idx]->GetSlice() + delta : 0;

    // 夹紧到有效范围
    newSlice = std::max(newSlice, m_minSlice[idx]);
    newSlice = std::min(newSlice, m_maxSlice[idx]);

    RequestSetSlice(view, newSlice);

    // 通知 Overlay Feature 更新坐标（测量标注随切片移动）
    if (m_renderers[idx]) {
        auto* mgr = m_renderers[idx]->GetOverlayManager();
        if (mgr) {
            mgr->OnSliceChanged(view, newSlice);
        }
        m_renderers[idx]->RequestRender();
    }
}

void ThreeViewController::UpdateSliceByWorldPoint(std::array<double, 3> worldPoint)
{
    if (!m_image || m_isUpdatingSlice) return;

    m_isUpdatingSlice = true;

    // 将世界坐标转换为体素索引（连续值）
    double ijk[3];
    double picked[3] = { worldPoint[0], worldPoint[1], worldPoint[2] };
    m_image->TransformPhysicalPointToContinuousIndex(picked, ijk);

    // 分别更新三个方向的切片
    SetSliceInternal(ViewType::Axial, static_cast<int>(std::round(ijk[2])));
    SetSliceInternal(ViewType::Sagittal, static_cast<int>(std::round(ijk[0])));
    SetSliceInternal(ViewType::Coronal, static_cast<int>(std::round(ijk[1])));

    m_isUpdatingSlice = false;
}

void ThreeViewController::SetWindowLevel(double window, double level)
{
    m_windowWidth = window;
    m_windowLevel = level;

    for (auto* r : m_renderers) {
        if (!r) continue;
        r->SetColorWindow(m_windowWidth);
        r->SetColorLevel(m_windowLevel);
        r->RequestRender();
    }
}

void ThreeViewController::Zoom(int viewIndex, double factor,
    std::array<double, 3> focalWorldPoint)
{
    auto renderer = GetRenderer(viewIndex);
    if (!renderer) return;

    auto camera = renderer->GetViewer()->GetRenderer()->GetActiveCamera();
    if (!camera || !camera->GetParallelProjection()) return;

    auto windowSize = renderer->GetViewer()->GetRenderWindow()->GetSize();
    if (windowSize[0] <= 0 || windowSize[1] <= 0) return;

    // 当前相机参数
    double oldFocal[3];
    camera->GetFocalPoint(oldFocal);

    double oldPos[3];
    camera->GetPosition(oldPos);

    // 计算缩放前后焦点与点击点的偏移
    double shift[3];
    for (int i = 0; i < 3; ++i) {
        shift[i] = focalWorldPoint[i] - oldFocal[i];
    }

    // 更新相机焦点和位置，让点击点保持在屏幕上不动
    double newFocal[3], newPos[3];
    for (int i = 0; i < 3; ++i) {
        newFocal[i] = oldFocal[i] + shift[i] * (1.0 - 1.0 / factor);
        newPos[i] = oldPos[i] + shift[i] * (1.0 - 1.0 / factor);
    }

    camera->SetFocalPoint(newFocal);
    camera->SetPosition(newPos);

    double newScale = camera->GetParallelScale() / factor;
    camera->SetParallelScale(newScale);
    renderer->GetViewer()->GetRenderer()->ResetCameraClippingRange();

    renderer->RequestRender();
}

// ============================================================
//  交互模式
// ============================================================

void ThreeViewController::SetInteractionMode(InteractionMode mode)
{
    if (m_currentMode == mode) return;

    UnregisterEventCallbacks();
    m_currentMode = mode;
    RegisterEventCallbacks();
}

// ============================================================
//  切片操作
// ============================================================

void ThreeViewController::RequestSetSlice(ViewType view, int slice)
{
    const int idx = static_cast<int>(view);
    if (!m_renderers[idx]) return;

    // 夹紧到有效范围
    slice = std::max(slice, m_minSlice[idx]);
    slice = std::min(slice, m_maxSlice[idx]);

    SetSliceInternal(view, slice);
    emit sliceChanged(idx, slice);
}

int ThreeViewController::GetSlice(ViewType view) const
{
    const int idx = static_cast<int>(view);
    if (!m_renderers[idx]) return 0;
    return m_renderers[idx]->GetSlice();
}

// ============================================================
//  其他控制
// ============================================================

std::array<double, 6> ThreeViewController::GetImageBounds() const
{
    if (!m_image) return {};

    double raw[6];
    const_cast<vtkImageData*>(m_image.Get())->GetBounds(raw);
    return { raw[0], raw[1], raw[2], raw[3], raw[4], raw[5] };
}

void ThreeViewController::ClearAllStrategyDrawings()
{
    auto it = m_strategies.find(m_currentMode);
    if (it == m_strategies.end() || !it->second) return;

    for (int i = 0; i < 3; ++i) {
        it->second->Clear(i);
        if (m_renderers[i]) {
            m_renderers[i]->RequestRender();
        }
    }
}

// ============================================================
//  私有方法
// ============================================================

void ThreeViewController::ComputeSliceRanges()
{
    if (!m_image) return;

    int dims[3];
    m_image->GetDimensions(dims);

    m_minSlice[static_cast<int>(ViewType::Axial)] = 0;
    m_maxSlice[static_cast<int>(ViewType::Axial)] = dims[2] - 1;

    m_minSlice[static_cast<int>(ViewType::Sagittal)] = 0;
    m_maxSlice[static_cast<int>(ViewType::Sagittal)] = dims[0] - 1;

    m_minSlice[static_cast<int>(ViewType::Coronal)] = 0;
    m_maxSlice[static_cast<int>(ViewType::Coronal)] = dims[1] - 1;
}

void ThreeViewController::SetSliceInternal(ViewType view, int slice)
{
    const int idx = static_cast<int>(view);
    if (!m_renderers[idx]) return;

    m_renderers[idx]->SetSlice(slice);
    // 传入最大切片数供 Overlay 信息层显示"当前/总数"
    m_renderers[idx]->SetMaxSlice(m_maxSlice[static_cast<int>(ViewType::Axial)]);
    m_renderers[idx]->RequestRender();
}

void ThreeViewController::RegisterEventCallbacks()
{
    for (int i = 0; i < 3; ++i) {
        if (!m_renderers[i]) continue;

        // 若此视图尚未创建 Overlay 管理器，现在创建并初始化
        if (!m_renderers[i]->GetOverlayManager()) {
            auto overlayMgr = OverlayFactory::CreateDefault();
            overlayMgr->SetImageWorldBounds(GetImageBounds());
            overlayMgr->Initialize(m_renderers[i]->GetOverlayRenderer(),
                m_renderers[i]->GetViewer());
            m_renderers[i]->SetOverlayManager(std::move(overlayMgr));
        }

        const int idx = i;

        // 滚轮事件：始终由 Normal 策略处理（切片导航不受模式影响）
        auto wheelHandler = [this, idx](EventType type) {
            return [this, idx, type](const EventData& data) {
                auto it = m_strategies.find(InteractionMode::Normal);
                if (it != m_strategies.end() && it->second) {
                    it->second->HandleEvent(type, idx, data);
                }
                };
            };

        // 其他事件：由当前激活模式的策略处理
        auto modeHandler = [this, idx](EventType type) {
            return [this, idx, type](const EventData& data) {
                auto it = m_strategies.find(m_currentMode);
                if (it != m_strategies.end() && it->second) {
                    it->second->HandleEvent(type, idx, data);
                }
                };
            };

        m_renderers[i]->OnEvent(EventType::WheelForward, wheelHandler(EventType::WheelForward));
        m_renderers[i]->OnEvent(EventType::WheelBackward, wheelHandler(EventType::WheelBackward));
        m_renderers[i]->OnEvent(EventType::LeftPress, modeHandler(EventType::LeftPress));
        m_renderers[i]->OnEvent(EventType::LeftMove, modeHandler(EventType::LeftMove));
        m_renderers[i]->OnEvent(EventType::LeftRelease, modeHandler(EventType::LeftRelease));
        m_renderers[i]->OnEvent(EventType::RightPress, modeHandler(EventType::RightPress));
        m_renderers[i]->OnEvent(EventType::KeyPress, modeHandler(EventType::KeyPress));
        m_renderers[i]->OnEvent(EventType::KeyRelease, modeHandler(EventType::KeyRelease));
    }
}

void ThreeViewController::UnregisterEventCallbacks()
{
    for (int i = 0; i < 3; ++i) {
        if (!m_renderers[i]) continue;
        m_renderers[i]->OnEvent(EventType::WheelForward, nullptr);
        m_renderers[i]->OnEvent(EventType::WheelBackward, nullptr);
        m_renderers[i]->OnEvent(EventType::LeftPress, nullptr);
        m_renderers[i]->OnEvent(EventType::LeftMove, nullptr);
        m_renderers[i]->OnEvent(EventType::LeftRelease, nullptr);
        m_renderers[i]->OnEvent(EventType::RightPress, nullptr);
        m_renderers[i]->OnEvent(EventType::KeyPress, nullptr);
        m_renderers[i]->OnEvent(EventType::KeyRelease, nullptr);
    }
}
