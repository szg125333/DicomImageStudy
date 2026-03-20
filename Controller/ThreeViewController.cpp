#include "ThreeViewController.h"
#include "Renderer/OverlayManager/IOverlayManager.h"
#include "Renderer/VtkViewRenderer.h"

#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkCellPicker.h>
#include <vtkCamera.h>
#include <vtkRenderWindow.h>

#include "Renderer/OverlayManager/OverlayFactory.h"
#include "Controller/Strategy/InteractionStrategyFactory.h"

ThreeViewController::ThreeViewController(QObject* parent) : QObject(parent) {
    m_renderers.fill(nullptr);
    for (int i = 0; i < 3; ++i) {
        m_minSlice[i] = 0;
        m_maxSlice[i] = 0;
    }

    m_strategies = InteractionStrategyFactory::CreateStrategies(this);
    SetInteractionMode(InteractionMode::Normal);
}

ThreeViewController::~ThreeViewController() {
}

void ThreeViewController::SetRenderers(std::array<IViewRenderer*, 3> renderers) {
    m_renderers = renderers;
}

void ThreeViewController::SetInteractionMode(InteractionMode mode) {
    if (m_CurrentMode == mode) return;

    unregisterEvents();
    m_CurrentMode = mode;
    registerEvents();
}

void ThreeViewController::SetImageData(vtkImageData* image) {
    if (!image) return;
    m_image = image;
    for (auto r : m_renderers) {
        if (r) r->SetInputData(image);
    }
    computeSliceRanges();

    int dims[3];
    m_image->GetDimensions(dims);
    double crossPoint[3];

    crossPoint[0] = dims[0] / 2.0;
    crossPoint[1] = dims[1] / 2.0;
    crossPoint[2] = dims[2] / 2.0;

    updateSliceInternal(ViewType::Axial, static_cast<int>(crossPoint[2]));
    updateSliceInternal(ViewType::Sagittal, static_cast<int>(crossPoint[0]));
    updateSliceInternal(ViewType::Coronal, static_cast<int>(crossPoint[1]));

    double range[2];
    m_image->GetScalarRange(range);

    m_windowWidth = range[1] - range[0];
    m_windowLevel = (range[0] + range[1]) / 2.0;

    for (int i = 0; i < m_renderers.size(); ++i) {
        m_renderers[i]->SetColorWindow(m_windowWidth);
        m_renderers[i]->SetColorLevel(m_windowLevel);
    }

    for (int i = 0; i < 3; ++i) {
        auto viewer = m_renderers[i]->GetViewer();
        if (viewer && viewer->GetRenderer()) {
            viewer->GetRenderer()->GetActiveCamera()->SetParallelProjection(true);
            viewer->GetRenderer()->ResetCamera();
        }
    }

    registerEvents();
}

void ThreeViewController::RequestSetSlice(ViewType view, int slice) {
    int idx = static_cast<int>(view);
    if (!m_renderers[idx]) return;
    if (slice < m_minSlice[idx]) slice = m_minSlice[idx];
    if (slice > m_maxSlice[idx]) slice = m_maxSlice[idx];

    if (m_internalUpdate) {
        updateSliceInternal(view, slice);
        return;
    }

    m_internalUpdate = true;
    updateSliceInternal(view, slice);
    emit sliceChanged(static_cast<int>(view), slice);

    m_internalUpdate = false;
}

int ThreeViewController::GetSlice(ViewType view) const {
    int idx = static_cast<int>(view);
    if (!m_renderers[idx]) return 0;
    return m_renderers[idx]->GetSlice();
}

void ThreeViewController::ChangeSlice(int viewIndex, int delta) {
    ViewType view = static_cast<ViewType>(viewIndex);
    int current = GetSlice(view);
    int newSlice = current + delta;

    if (newSlice < m_minSlice[static_cast<int>(view)]) newSlice = m_minSlice[static_cast<int>(view)];
    if (newSlice > m_maxSlice[static_cast<int>(view)]) newSlice = m_maxSlice[static_cast<int>(view)];

    RequestSetSlice(view, newSlice);
    if (m_renderers[viewIndex]) {
		m_renderers[viewIndex]->GetOverlayManager()->OnSliceChanged(static_cast<ViewType>(viewIndex),newSlice);
        m_renderers[viewIndex]->RequestRender();
    }
}

void ThreeViewController::UpdateSliceInternals(std::array<double, 3> worldPoint) {
    double ijk[3];
    double picked[3] = { worldPoint[0], worldPoint[1], worldPoint[2] };
    m_image->TransformPhysicalPointToContinuousIndex(picked, ijk);
    updateSliceInternal(ViewType::Axial, static_cast<int>(std::round(ijk[2])));
    updateSliceInternal(ViewType::Sagittal, static_cast<int>(std::round(ijk[0])));
    updateSliceInternal(ViewType::Coronal, static_cast<int>(std::round(ijk[1])));
}

const vtkImageData* ThreeViewController::GetImage() const
{
    return m_image.Get();
}

void ThreeViewController::SetWindowLevel(double ww, double wl) {
    m_windowWidth = ww;
    m_windowLevel = wl;

    for (int i = 0; i < 3; ++i) {
        if (!m_renderers[i]) continue;
        auto viewer = m_renderers[i]->GetViewer();
        if (viewer) {
            m_renderers[i]->SetColorWindow(m_windowWidth);
            m_renderers[i]->SetColorLevel(m_windowLevel);
        }
        m_renderers[i]->RequestRender();
    }
}

std::array<double, 6> ThreeViewController::GetImageBounds() const
{
    if (m_image) {
        double rawBounds[6];
        m_image->GetBounds(rawBounds);

        // 转为 std::array
        std::array<double, 6> bounds = {
            rawBounds[0], rawBounds[1],
            rawBounds[2], rawBounds[3],
            rawBounds[4], rawBounds[5]
        };

		return bounds;
    }
    return std::array<double, 6>();
}

void ThreeViewController::resetStrategyDrawings()
{
    for (int i = 0; i < m_renderers.size(); i++)
    {
        auto it = m_strategies.find(m_CurrentMode);
        if (it != m_strategies.end() && it->second) {
			it->second->Clear(i);
        }
        m_renderers[i]->RequestRender();
    }
}

void ThreeViewController::updateSliceInternal(ViewType view, int slice) {
    int idx = static_cast<int>(view);
    if (m_renderers[idx]) {
        m_renderers[idx]->SetSlice(slice);
        m_renderers[idx]->SetMaxSlice(m_maxSlice[static_cast<int>(ViewType::Axial)]);
        m_renderers[idx]->RequestRender();
    }
}

void ThreeViewController::computeSliceRanges() {
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

void ThreeViewController::registerEvents() {

    for (int i = 0; i < 3; ++i) {
        if (!m_renderers[i]) continue;
        int idx = i;

        auto forwardEvent = [this, idx](EventType type) {
            return [this, idx, type](const EventData& data) {
                    if(type == EventType::WheelForward || type == EventType::WheelBackward) {
                        auto it = m_strategies.find(InteractionMode::Normal);
                        if (it != m_strategies.end() && it->second) {
                            it->second->HandleEvent(type, idx, data);
                        }
                    }
                    else
                    {
                        auto it = m_strategies.find(m_CurrentMode);
                        if (it != m_strategies.end() && it->second) {
                            it->second->HandleEvent(type, idx, data);
                        }
                    }
                };
            };

        m_renderers[i]->OnEvent(EventType::WheelForward, forwardEvent(EventType::WheelForward));
        m_renderers[i]->OnEvent(EventType::WheelBackward, forwardEvent(EventType::WheelBackward));
        m_renderers[i]->OnEvent(EventType::LeftPress, forwardEvent(EventType::LeftPress));
        m_renderers[i]->OnEvent(EventType::LeftMove, forwardEvent(EventType::LeftMove));
        m_renderers[i]->OnEvent(EventType::LeftRelease, forwardEvent(EventType::LeftRelease));
        m_renderers[i]->OnEvent(EventType::RightPress, forwardEvent(EventType::RightPress));
        m_renderers[i]->OnEvent(EventType::KeyPress, forwardEvent(EventType::KeyPress));
        m_renderers[i]->OnEvent(EventType::KeyRelease, forwardEvent(EventType::KeyRelease));

        if (m_renderers[i]->GetOverlayManager() == nullptr) {
            auto overlayMgr = OverlayFactory::CreateDefault();
            overlayMgr->SetImageWorldBounds(GetImageBounds());
            overlayMgr->Initialize(m_renderers[i]->GetOverlayRenderer(), m_renderers[i]->GetViewer());
            m_renderers[i]->SetOverlayManager(std::move(overlayMgr));
        }
    }
}

void ThreeViewController::unregisterEvents() {
    for (int i = 0; i < 3; ++i) {
        if (!m_renderers[i]) continue;
        m_renderers[i]->OnEvent(EventType::WheelForward, nullptr);
        m_renderers[i]->OnEvent(EventType::WheelBackward, nullptr);
        m_renderers[i]->OnEvent(EventType::LeftPress, nullptr);
        m_renderers[i]->OnEvent(EventType::LeftMove, nullptr);
        m_renderers[i]->OnEvent(EventType::LeftRelease, nullptr);
        m_renderers[i]->OnEvent(EventType::RightPress, nullptr);
    }
}

void ThreeViewController::Zoom(int viewIndex, double factor, std::array<double, 3> initialFocalPoint)
{
    auto renderer = GetRenderer(viewIndex);
    if (!renderer) return;

    auto camera = renderer->GetViewer()->GetRenderer()->GetActiveCamera();
    if (!camera || !camera->GetParallelProjection()) return;

    auto windowSize=renderer->GetViewer()->GetRenderWindow()->GetSize();
    if (windowSize[0] <= 0 || windowSize[1] <= 0) return;

    // 当前相机参数
    double oldFocal[3];
    camera->GetFocalPoint(oldFocal);

    double oldPos[3];
    camera->GetPosition(oldPos);

    // 计算缩放前后焦点与点击点的偏移
    double shift[3];
    for (int i = 0; i < 3; ++i) {
        shift[i] = initialFocalPoint[i] - oldFocal[i];
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