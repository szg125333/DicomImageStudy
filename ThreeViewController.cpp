#include "ThreeViewController.h"
#include "NormalStrategy.h"
#include "IOverlayManager.h"
#include "SliceStrategy.h"
#include "ZoomStrategy.h"
#include "VtkViewRenderer.h"
#include "WindowLevelStrategy.h"
#include "SimpleCrosshairManager.h"
#include "SimpleWindowLevelManager.h"
#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkPropPicker.h>
#include <vtkCellPicker.h>
#include <vtkCamera.h>
#include <QDebug>
#include <QCoreApplication>
#include <QThread>

ThreeViewController::ThreeViewController(QObject* parent) : QObject(parent) {
    m_renderers.fill(nullptr);
    for (int i = 0; i < 3; ++i) {
        m_minSlice[i] = 0;
        m_maxSlice[i] = 0;
    }

    SetInteractionMode(InteractionMode::Normal);
}

static void ShutdownCrosshairsImpl(std::array<std::unique_ptr<ICrosshairManager>, 3>& managers) {
    for (int i = 0; i < 3; ++i) {
        if (managers[i]) {
            // 从 renderer 中移除 actor 等清理工作
            managers[i]->Shutdown();
            // 释放对象（unique_ptr 在离开作用域时也会释放，但这里显式 reset 更直观）
            managers[i].reset();
        }
    }
}

static void ShutdownCrosshairsImpl(std::array<std::unique_ptr<IWindowLevelManager>, 3>& managers) {
    for (int i = 0; i < 3; ++i) {
        if (managers[i]) {
            // 从 renderer 中移除 actor 等清理工作
            managers[i]->Shutdown();
            // 释放对象（unique_ptr 在离开作用域时也会释放，但这里显式 reset 更直观）
            managers[i].reset();
        }
    }
}

ThreeViewController::~ThreeViewController() {
    //// 如果当前线程就是主线程（Qt GUI 线程），直接清理
    //QThread* guiThread = QCoreApplication::instance() ? QCoreApplication::instance()->thread() : nullptr;
    //if (QThread::currentThread() == guiThread) {
    //    ShutdownCrosshairsImpl(m_crosshairManagers);
    //    ShutdownCrosshairsImpl(m_windowLevelManagers);
    //    return;
    //}

    //// 否则，调度到主线程同步执行清理，确保在析构返回前完成
    //// 使用 QMetaObject::invokeMethod + BlockingQueuedConnection 保证同步执行
    //bool invoked = QMetaObject::invokeMethod(
    //    QCoreApplication::instance(),
    //    // functor 支持 Qt5.10+；如果你的 Qt 版本较旧，请改为使用槽函数或信号/槽桥接
    //    [this]() {
    //        ShutdownCrosshairsImpl(m_crosshairManagers);
    //        ShutdownCrosshairsImpl(m_windowLevelManagers);
    //    },
    //    Qt::BlockingQueuedConnection
    //);

    //if (!invoked) {
    //    // 如果调度失败（极少见），作为兜底在当前线程尝试清理（谨慎）
    //    ShutdownCrosshairsImpl(m_crosshairManagers);
    //    ShutdownCrosshairsImpl(m_windowLevelManagers);
    //}
}

void ThreeViewController::SetRenderers(std::array<IViewRenderer*, 3> renderers) {
    m_renderers = renderers;
    registerEvents(); // 初始注册

    //// 初始化 crosshair managers（如果尚未）
    //for (int i = 0; i < 3; ++i) {
    //    if (!m_renderers[i]) continue;
    //    if (!m_crosshairManagers[i]) {
    //        m_crosshairManagers[i] = std::make_unique<SimpleCrosshairManager>();
    //        // 使用 overlay renderer（你已有的 GetOverlayRenderer）
    //        auto overlayRen = dynamic_cast<VtkViewRenderer*>(m_renderers[i])->GetOverlayRenderer();
    //        if (overlayRen) {
    //            m_crosshairManagers[i]->Initialize(overlayRen);
    //        }
    //    }

    //    if (!m_windowLevelManagers[i]) {
    //        m_windowLevelManagers[i] = std::make_unique<SimpleWindowLevelManager>();
    //        auto overlayRen = dynamic_cast<VtkViewRenderer*>(m_renderers[i])->GetOverlayRenderer();
    //        if (overlayRen) m_windowLevelManagers[i]->Initialize(overlayRen, m_renderers[i]->GetViewer());
    //    }
    //}


}

void ThreeViewController::SetInteractionMode(InteractionMode mode) {
    if (m_mode == mode) return; // 模式未变化则不处理

    // 移除旧模式事件
    unregisterEvents();

    // 更新模式
    m_mode = mode;
    switch (mode) {
    case InteractionMode::Normal:
        m_strategy = std::make_unique<NormalStrategy>(this);
        break;
    case InteractionMode::Slice:
        m_strategy = std::make_unique<SliceStrategy>(this);
        break;
    case InteractionMode::Zoom:
        m_strategy = std::make_unique<ZoomStrategy>(this);
        break;
    case InteractionMode::WindowLevel: 
        m_strategy = std::make_unique<WindowLevelStrategy>(this); break;
    default:
        m_strategy.reset();
        break;
    }

    // 注册新模式事件
    registerEvents();
}

void ThreeViewController::registerEvents() {
    for (int i = 0; i < 3; ++i) {
        if (!m_renderers[i]) continue;
        int idx = i;

        m_renderers[i]->OnEvent(EventType::WheelForward, [this, idx](void* data) {
            if (m_strategy) m_strategy->HandleEvent(EventType::WheelForward, idx, data);
            });
        m_renderers[i]->OnEvent(EventType::WheelBackward, [this, idx](void* data) {
            if (m_strategy) m_strategy->HandleEvent(EventType::WheelBackward, idx, data);
            });
        m_renderers[i]->OnEvent(EventType::LeftPress, [this, idx](void* data) {
            if (m_strategy) m_strategy->HandleEvent(EventType::LeftPress, idx, data);
            });
        m_renderers[i]->OnEvent(EventType::RightClick, [this, idx](void* data) {
            if (m_strategy) m_strategy->HandleEvent(EventType::RightClick, idx, data);
            });
        m_renderers[i]->OnEvent(EventType::LeftMove, [this, idx](void* data) {
            if (m_strategy) m_strategy->HandleEvent(EventType::LeftMove, idx, data);
            });
        m_renderers[i]->OnEvent(EventType::LeftRelease, [this, idx](void* data) {
            if (m_strategy) m_strategy->HandleEvent(EventType::LeftRelease, idx, data);
            });
    }
}

void ThreeViewController::unregisterEvents() {
    for (int i = 0; i < 3; ++i) {
        if (!m_renderers[i]) continue;
        m_renderers[i]->OnEvent(EventType::WheelForward, nullptr);
        m_renderers[i]->OnEvent(EventType::WheelBackward, nullptr);
        m_renderers[i]->OnEvent(EventType::LeftPress, nullptr);
        m_renderers[i]->OnEvent(EventType::RightClick, nullptr);
    }
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
    double m_crossPoint[3];

    // 初始化交叉点到图像中心
    m_crossPoint[0] = dims[0] / 2; // x
    m_crossPoint[1] = dims[1] / 2; // y
    m_crossPoint[2] = dims[2] / 2; // z

    // 更新三视图到交叉点
    updateSliceInternal(ViewType::Axial, m_crossPoint[2]);
    updateSliceInternal(ViewType::Sagittal, m_crossPoint[0]);
    updateSliceInternal(ViewType::Coronal, m_crossPoint[1]);

    double range[2];
    m_image->GetScalarRange(range); // 获取像素值范围 [min, max]

    m_windowWidth = range[1] - range[0];   // 窗宽 = 最大值 - 最小值
    m_windowLevel = (range[0] + range[1]) / 2.0; // 窗位 = 中心值

    for (int i = 0; i < m_renderers.size(); ++i) {
        auto viewer = m_renderers[i]->GetViewer();
        viewer->SetColorWindow(m_windowWidth);
        viewer->SetColorLevel(m_windowLevel);
    }

    // === 新增：设置所有视图为平行投影 ===
    for (int i = 0; i < 3; ++i) {
        auto viewer = m_renderers[i]->GetViewer();
        if (viewer && viewer->GetRenderer()) {
            viewer->GetRenderer()->GetActiveCamera()->SetParallelProjection(true);
            viewer->GetRenderer()->ResetCamera();

        }
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

void ThreeViewController::updateSliceInternal(ViewType view, int slice) {
    int idx = static_cast<int>(view);
    if (m_renderers[idx]) {
        m_renderers[idx]->SetSlice(slice);
        m_renderers[idx]->RequestRender();
    }
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

    // 边界检查
    if (newSlice < m_minSlice[static_cast<int>(view)]) newSlice = m_minSlice[static_cast<int>(view)];
    if (newSlice > m_maxSlice[static_cast<int>(view)]) newSlice = m_maxSlice[static_cast<int>(view)];

    // 调用 RequestSetSlice 来更新并同步
    RequestSetSlice(view, newSlice);
}

void ThreeViewController::LocatePoint(int viewIndex, int* pos) {
    auto viewer = m_renderers[viewIndex]->GetViewer();
    if (!viewer || !m_image) return;

    vtkRenderer* ren = viewer->GetRenderer();
    if (!ren) return;

    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.001); // 容差可调
    double picked[3];
    if (picker->Pick(pos[0], pos[1], 0, ren)) {
        picker->GetPickPosition(picked);
        qDebug() << "Picked world coordinates:" << picked[0] << picked[1] << picked[2];
    }
    else
    {
		return; // 没有拾取到有效位置，直接返回
    }

    // Step 4: 转为 IJK（连续值），用于更新其他视图的 slice
    double ijk[3];
    m_image->TransformPhysicalPointToContinuousIndex(picked, ijk);

    // 更新三视图 slice（取整）
    updateSliceInternal(ViewType::Axial, static_cast<int>(std::round(ijk[2])));
    updateSliceInternal(ViewType::Sagittal, static_cast<int>(std::round(ijk[0])));
    updateSliceInternal(ViewType::Coronal, static_cast<int>(std::round(ijk[1])));

    // Step 5: 更新十字线
	std::array<double, 3> worldPoint = { picked[0], picked[1], picked[2] };
    UpdateCrosshairInAllViews(worldPoint);
}

void ThreeViewController::UpdateCrosshairInAllViews(std::array<double, 3> worldPoint) {
    if (!m_image) return;

    int dims[3];
    m_image->GetDimensions(dims);
    double spacing[3], origin[3];
    m_image->GetSpacing(spacing);
    m_image->GetOrigin(origin);

    std::array<double, 3> worldMin, worldMax;
    for (int j = 0; j < 3; ++j) {
        worldMin[j] = origin[j];
        worldMax[j] = origin[j] + (dims[j] - 1) * spacing[j];
    }

    //double worldPoint[3] = { picked[0], picked[1], picked[2] };

    //std::array<double, 3> worldMin
    for (int i = 0; i < 3; ++i) {
        if (!m_renderers[i]) continue;

        m_renderers[i]->GetOverlayManager()->UpdateCrosshair(worldPoint,
            static_cast<ViewType>(i),
            worldMin,
            worldMax);
        //// 直接交给 manager 计算端点并更新
        //if (m_crosshairManagers[i]) {
        //    m_crosshairManagers[i]->UpdateCrosshair(worldPoint,
        //        static_cast<ViewType>(i),
        //        worldMin,
        //        worldMax);
        //}

        // 渲染：建议改为 RequestRender，由 RenderWindowManager 合并；这里保留兼容调用
        m_renderers[i]->RequestRender();
        //if (viewer) viewer->Render();
    }
}

void ThreeViewController::SetWindowLevel(double ww, double wl) {
    m_windowWidth = ww;
    m_windowLevel = wl;
    for (int i = 0; i < 3; ++i) {
        m_renderers[i]->GetOverlayManager()->SetWindowLevel(ww, wl);
        auto viewer = m_renderers[i]->GetViewer();
        if (viewer) viewer->Render();
    }
    //vtkRenderer->RequestRender();
}

void ThreeViewController::syncCameras() {
    auto refCam = m_renderers[static_cast<int>(ViewType::Axial)]->GetViewer()->GetRenderer()->GetActiveCamera();
    double scale = refCam->GetParallelScale();

    for (int i = 0; i < 3; ++i) {
        auto cam = m_renderers[i]->GetViewer()->GetRenderer()->GetActiveCamera();
        cam->SetParallelScale(scale);
        m_renderers[i]->GetViewer()->GetRenderer()->ResetCameraClippingRange();
    }
}