#include "ThreeViewController.h"
#include "Controller/Strategy/NormalStrategy.h"
#include "Controller/Strategy/WindowLevelStrategy.h"
#include "Controller/Strategy/DistanceMeasureStrategy.h"
#include "Renderer/OverlayManager/IOverlayManager.h"
#include "Renderer/VtkViewRenderer.h"

#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkCellPicker.h>
#include <vtkCamera.h>
#include <vtkPropPicker.h>

#include "Renderer/OverlayManager/OverlayFactory.h"           // ← 轻量工厂头文件
#include "Renderer/OverlayManager/CrosshairManager/SimpleCrosshairManager.h"
#include "Renderer/OverlayManager/DistanceMeasureManager/SimpleDistanceMeasureManager.h"

ThreeViewController::ThreeViewController(QObject* parent) : QObject(parent) {
    m_renderers.fill(nullptr);
    for (int i = 0; i < 3; ++i) {
        m_minSlice[i] = 0;
        m_maxSlice[i] = 0;
    }

    SetInteractionMode(InteractionMode::Normal);
}
ThreeViewController::~ThreeViewController() {
}

void ThreeViewController::SetRenderers(std::array<IViewRenderer*, 3> renderers) {
    m_renderers = renderers;
    registerEvents();
}

void ThreeViewController::SetInteractionMode(InteractionMode mode) {
    if (m_mode == mode) return;

    unregisterEvents();

    m_mode = mode;
    switch (mode) {
    case InteractionMode::Normal:
        m_strategy = std::make_unique<NormalStrategy>(this);
        break;
    case InteractionMode::Checkboard:
        // TODO: 实现棋盘格对比模式
        m_strategy.reset();
        break;
    case InteractionMode::ManualMove:
        // TODO: 实现手动平移/旋转模式
        m_strategy.reset();
        break;
    case InteractionMode::DistanceMeasure:
        // TODO: 实现距离测量工具
        m_strategy = std::make_unique<DistanceMeasureStrategy>(this);
        break;
    case InteractionMode::AngleMeasure:
        // TODO: 实现角度测量工具
        m_strategy.reset();
        break;
    case InteractionMode::ContourMeasure:
        // TODO: 实现轮廓测量模式
        m_strategy.reset();
        break;
    case InteractionMode::RegistrationROI:
        // TODO: 实现配准 ROI 模式
        m_strategy.reset();
        break;
    case InteractionMode::HandIrregularContour:
        // TODO: 实现手工不规则轮廓模式
        m_strategy.reset();
        break;
    case InteractionMode::None:
    default:
        m_strategy.reset();
        break;
    }

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
        auto viewer = m_renderers[i]->GetViewer();
        if (viewer) {
            viewer->SetColorWindow(m_windowWidth);
            viewer->SetColorLevel(m_windowLevel);
        }
    }

    for (int i = 0; i < 3; ++i) {
        auto viewer = m_renderers[i]->GetViewer();
        if (viewer && viewer->GetRenderer()) {
            viewer->GetRenderer()->GetActiveCamera()->SetParallelProjection(true);
            viewer->GetRenderer()->ResetCamera();
        }
    }
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
}

void ThreeViewController::LocatePoint(int viewIndex, int* pos) {
    auto viewer = m_renderers[viewIndex]->GetViewer();
    if (!viewer || !m_image) return;

    vtkRenderer* ren = viewer->GetRenderer();
    if (!ren) return;

    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.001);
    double picked[3];
    if (picker->Pick(pos[0], pos[1], 0, ren)) {
        picker->GetPickPosition(picked);
    }
    else {
        return;
    }
    std::array<double, 3> worldPoint = { picked[0], picked[1], picked[2] };

    UpdateCrosshairInAllViews(worldPoint);

    double ijk[3];
    m_image->TransformPhysicalPointToContinuousIndex(picked, ijk);
    updateSliceInternal(ViewType::Axial, static_cast<int>(std::round(ijk[2])));
    updateSliceInternal(ViewType::Sagittal, static_cast<int>(std::round(ijk[0])));
    updateSliceInternal(ViewType::Coronal, static_cast<int>(std::round(ijk[1])));
}

void ThreeViewController::UpdateCrosshairInAllViews(std::array<double, 3> worldPoint) {
    if (!m_image) return;

    int dims[3];
    double spacing[3], origin[3];

    m_image->GetDimensions(dims);
    m_image->GetSpacing(spacing);
    m_image->GetOrigin(origin);

    std::array<double, 3> worldMin, worldMax;
    for (int j = 0; j < 3; ++j) {
        worldMin[j] = origin[j];
        worldMax[j] = origin[j] + (dims[j] - 1) * spacing[j];
    }

    for (int i = 0; i < 3; ++i) {
        if (!m_renderers[i]) continue;

        auto overlayManager = m_renderers[i]->GetOverlayManager();
        if (overlayManager) {
            overlayManager->GetFeature<SimpleCrosshairManager>()->UpdateCrosshair(worldPoint,
                static_cast<ViewType>(i),
                worldMin.data(),
                worldMax.data());
        }

        m_renderers[i]->RequestRender();
    }
}

void ThreeViewController::SetWindowLevel(double ww, double wl) {
    m_windowWidth = ww;
    m_windowLevel = wl;

    for (int i = 0; i < 3; ++i) {
        if (!m_renderers[i]) continue;
        auto viewer = m_renderers[i]->GetViewer();
        if (viewer) {
            viewer->SetColorWindow(m_windowWidth);
            viewer->SetColorLevel(m_windowLevel);
        }
        m_renderers[i]->RequestRender();
    }
}

void ThreeViewController::updateSliceInternal(ViewType view, int slice) {
    int idx = static_cast<int>(view);
    if (m_renderers[idx]) {
        m_renderers[idx]->SetSlice(slice);
        m_renderers[idx]->RequestRender();
    }
}

void ThreeViewController::computeSliceRanges() {
    if (!m_image) return;
    int dims[3];
    m_image->GetDimensions(dims);
    m_minSlice[0] = 0; m_maxSlice[0] = dims[2] - 1;
    m_minSlice[1] = 0; m_maxSlice[1] = dims[0] - 1;
    m_minSlice[2] = 0; m_maxSlice[2] = dims[1] - 1;
}

void ThreeViewController::registerEvents() {

    for (int i = 0; i < 3; ++i) {
        if (!m_renderers[i]) continue;
        int idx = i;

        // 在你的类中（比如 ViewController.cpp）
        auto forwardEvent = [this, idx](EventType type) {
            return [this, idx, type](const EventData& data) {
                if (m_strategy) {
                    m_strategy->HandleEvent(type, idx, data);
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

        auto overlayMgr = CreateDefaultOverlayManager();
        m_renderers[i]->SetOverlayManager(std::move(overlayMgr));
        m_renderers[i]->GetOverlayManager()->Initialize(m_renderers[i]->GetOverlayRenderer(), m_renderers[i]->GetViewer());
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