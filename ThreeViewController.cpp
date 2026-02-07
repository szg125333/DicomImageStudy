#include "ThreeViewController.h"
#include "SliceStrategy.h"
#include "ZoomStrategy.h"
#include <vtkImageData.h>
#include <QDebug>

ThreeViewController::ThreeViewController(QObject* parent) : QObject(parent) {
    m_renderers.fill(nullptr);
    for (int i = 0; i < 3; ++i) {
        m_minSlice[i] = 0;
        m_maxSlice[i] = 0;
    }
}

void ThreeViewController::SetRenderers(std::array<IViewRenderer*, 3> renderers) {
    m_renderers = renderers;
    for (int i = 0; i < 3; ++i) {
        if (m_renderers[i]) {
            int idx = i;
            // 统一事件绑定，转发给当前策略
            m_renderers[i]->OnEvent(EventType::WheelForward, [this, idx]() {
                if (m_strategy) m_strategy->HandleEvent(EventType::WheelForward, idx);
                });
            m_renderers[i]->OnEvent(EventType::WheelBackward, [this, idx]() {
                if (m_strategy) m_strategy->HandleEvent(EventType::WheelBackward, idx);
                });
            m_renderers[i]->OnEvent(EventType::LeftClick, [this, idx]() {
                if (m_strategy) m_strategy->HandleEvent(EventType::LeftClick, idx);
                });
            m_renderers[i]->OnEvent(EventType::RightClick, [this, idx]() {
                if (m_strategy) m_strategy->HandleEvent(EventType::RightClick, idx);
                });
        }
    }
}

void ThreeViewController::SetInteractionMode(InteractionMode mode) {
    m_mode = mode;
    switch (mode) {
    case InteractionMode::Slice:
        m_strategy = std::make_unique<SliceStrategy>(this);
        break;
    case InteractionMode::Zoom:
        m_strategy = std::make_unique<ZoomStrategy>(this);
        break;
    default:
        m_strategy.reset(); // 没有策略
        break;
    }
}

void ThreeViewController::SetImageData(vtkImageData* image) {
    if (!image) return;
    m_image = image;
    for (auto r : m_renderers) {
        if (r) r->SetInputData(image);
    }
    computeSliceRanges();
    for (int i = 0; i < 3; ++i) {
        int mid = (m_minSlice[i] + m_maxSlice[i]) / 2;
        updateSliceInternal(static_cast<ViewType>(i), mid);
    }
}

void ThreeViewController::computeSliceRanges() {
    if (!m_image) return;
    int dims[3];
    m_image->GetDimensions(dims);
    m_minSlice[0] = 0; m_maxSlice[0] = dims[2] - 1; // axial z
    m_minSlice[1] = 0; m_maxSlice[1] = dims[0] - 1; // sagittal x
    m_minSlice[2] = 0; m_maxSlice[2] = dims[1] - 1; // coronal y
}

void ThreeViewController::RequestSetSlice(ViewType view, int slice) {
    if (!m_renderers[view]) return;
    if (slice < m_minSlice[view]) slice = m_minSlice[view];
    if (slice > m_maxSlice[view]) slice = m_maxSlice[view];

    if (m_internalUpdate) {
        updateSliceInternal(view, slice);
        return;
    }

    m_internalUpdate = true;
    updateSliceInternal(view, slice);
    syncFrom(view, slice);
    emit sliceChanged(static_cast<int>(view), slice);
    m_internalUpdate = false;
}

void ThreeViewController::updateSliceInternal(ViewType view, int slice) {
    if (m_renderers[view]) {
        m_renderers[view]->SetSlice(slice);
        m_renderers[view]->Render();
    }
}

int ThreeViewController::GetSlice(ViewType view) const {
    if (!m_renderers[view]) return 0;
    return m_renderers[view]->GetSlice();
}

void ThreeViewController::syncFrom(ViewType srcView, int srcSlice) {
    if (!m_image) return;
    int dims[3];
    m_image->GetDimensions(dims);

    int targetSlices[3] = { 0,0,0 };
    if (srcView == Axial) {
        targetSlices[Axial] = srcSlice;
        targetSlices[Sagittal] = dims[0] / 2;
        targetSlices[Coronal] = dims[1] / 2;
    }
    else if (srcView == Sagittal) {
        targetSlices[Sagittal] = srcSlice;
        targetSlices[Axial] = dims[2] / 2;
        targetSlices[Coronal] = dims[1] / 2;
    }
    else {
        targetSlices[Coronal] = srcSlice;
        targetSlices[Axial] = dims[2] / 2;
        targetSlices[Sagittal] = dims[0] / 2;
    }

    for (int v = 0; v < 3; ++v) {
        if (v == srcView) continue;
        int s = targetSlices[v];
        if (s < m_minSlice[v]) s = m_minSlice[v];
        if (s > m_maxSlice[v]) s = m_maxSlice[v];
        updateSliceInternal(static_cast<ViewType>(v), s);
        emit sliceChanged(v, s);
    }
}
