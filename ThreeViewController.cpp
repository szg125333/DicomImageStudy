#include "ThreeViewController.h"
#include "NormalStrategy.h"
#include "SliceStrategy.h"
#include "ZoomStrategy.h"
#include "VtkViewRenderer.h"
#include "WindowLevelStrategy.h"
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

ThreeViewController::ThreeViewController(QObject* parent) : QObject(parent) {
    m_renderers.fill(nullptr);
    for (int i = 0; i < 3; ++i) {
        m_minSlice[i] = 0;
        m_maxSlice[i] = 0;
    }

    SetInteractionMode(InteractionMode::Normal);
}

void ThreeViewController::SetRenderers(std::array<IViewRenderer*, 3> renderers) {
    m_renderers = renderers;
    registerEvents(); // 初始注册
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

    // 初始化交叉点到图像中心
    m_crossPoint[0] = dims[0] / 2; // x
    m_crossPoint[1] = dims[1] / 2; // y
    m_crossPoint[2] = dims[2] / 2; // z

    // 更新三视图到交叉点
    updateSliceInternal(Axial, m_crossPoint[2]);
    updateSliceInternal(Sagittal, m_crossPoint[0]);
    updateSliceInternal(Coronal, m_crossPoint[1]);

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

    // 根据源视图更新交叉点坐标
    if (srcView == Axial) {
        m_crossPoint[2] = srcSlice; // z
    }
    else if (srcView == Sagittal) {
        m_crossPoint[0] = srcSlice; // x
    }
    else if (srcView == Coronal) {
        m_crossPoint[1] = srcSlice; // y
    }

    // --- 显式边界检查 ---
    // X (Sagittal)
    if (m_crossPoint[0] < m_minSlice[Sagittal]) {
        m_crossPoint[0] = m_minSlice[Sagittal];
    }
    else if (m_crossPoint[0] > m_maxSlice[Sagittal]) {
        m_crossPoint[0] = m_maxSlice[Sagittal];
    }

    // Y (Coronal)
    if (m_crossPoint[1] < m_minSlice[Coronal]) {
        m_crossPoint[1] = m_minSlice[Coronal];
    }
    else if (m_crossPoint[1] > m_maxSlice[Coronal]) {
        m_crossPoint[1] = m_maxSlice[Coronal];
    }

    // Z (Axial)
    if (m_crossPoint[2] < m_minSlice[Axial]) {
        m_crossPoint[2] = m_minSlice[Axial];
    }
    else if (m_crossPoint[2] > m_maxSlice[Axial]) {
        m_crossPoint[2] = m_maxSlice[Axial];
    }
}

void ThreeViewController::ChangeSlice(int viewIndex, int delta) {
    ViewType view = static_cast<ViewType>(viewIndex);
    int current = GetSlice(view);
    int newSlice = current + delta;

    // 边界检查
    if (newSlice < m_minSlice[view]) newSlice = m_minSlice[view];
    if (newSlice > m_maxSlice[view]) newSlice = m_maxSlice[view];

    // 更新交叉点坐标
    if (view == Axial)      m_crossPoint[2] = newSlice; // z
    else if (view == Sagittal) m_crossPoint[0] = newSlice; // x
    else if (view == Coronal)  m_crossPoint[1] = newSlice; // y

    // 调用 RequestSetSlice 来更新并同步
    RequestSetSlice(view, newSlice);
}

// LocatePoint: 从 display -> world -> ijk -> physical(world/mm)，保存到 m_crossPoint
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

    // Step 2: 根据当前视图类型，强制修正对应坐标轴，使其严格落在当前 slice 平面上
    double origin[3], spacing[3];
    m_image->GetOrigin(origin);
    m_image->GetSpacing(spacing);
    int currentSlice = viewer->GetSlice();

    // Step 3: 存储物理坐标（用于十字线渲染）
    m_crossPoint[0] = picked[0];
    m_crossPoint[1] = picked[1];
    m_crossPoint[2] = picked[2];

    qDebug() << "CrossPoint (world/physical):"
        << m_crossPoint[0] << m_crossPoint[1] << m_crossPoint[2];

    // Step 4: 转为 IJK（连续值），用于更新其他视图的 slice
    double ijk[3];
    m_image->TransformPhysicalPointToContinuousIndex(picked, ijk);

    // 更新三视图 slice（取整）
    updateSliceInternal(Axial, static_cast<int>(std::round(ijk[2])));
    updateSliceInternal(Sagittal, static_cast<int>(std::round(ijk[0])));
    updateSliceInternal(Coronal, static_cast<int>(std::round(ijk[1])));

    // Step 5: 更新十字线
    UpdateCrosshairInAllViews();
}

void ThreeViewController::UpdateCrosshairInAllViews() {
    if (!m_image) return;

    int dims[3];
    m_image->GetDimensions(dims);
    double spacing[3], origin[3];
    m_image->GetSpacing(spacing);
    m_image->GetOrigin(origin);

    double worldMin[3], worldMax[3];
    for (int j = 0; j < 3; ++j) {
        worldMin[j] = origin[j];
        worldMax[j] = origin[j] + (dims[j] - 1) * spacing[j];
    }

    for (int i = 0; i < m_renderers.size(); ++i) {
        auto viewer = m_renderers[i]->GetViewer();
        if (!viewer) continue;
        vtkRenderer* ren = viewer->GetRenderer();

        auto overlayRen = dynamic_cast<VtkViewRenderer*>(m_renderers[i])->GetOverlayRenderer();

        if (!m_crossActorsH3D[i]) {
            m_crossLinesH3D[i] = vtkSmartPointer<vtkLineSource>::New();
            auto hMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            hMapper->SetInputConnection(m_crossLinesH3D[i]->GetOutputPort());
            m_crossActorsH3D[i] = vtkSmartPointer<vtkActor>::New();
            m_crossActorsH3D[i]->SetMapper(hMapper);
            m_crossActorsH3D[i]->GetProperty()->SetColor(0, 1, 0);
            overlayRen->AddActor(m_crossActorsH3D[i]);

            m_crossLinesV3D[i] = vtkSmartPointer<vtkLineSource>::New();
            auto vMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            vMapper->SetInputConnection(m_crossLinesV3D[i]->GetOutputPort());
            m_crossActorsV3D[i] = vtkSmartPointer<vtkActor>::New();
            m_crossActorsV3D[i]->SetMapper(vMapper);
            m_crossActorsV3D[i]->GetProperty()->SetColor(0, 1, 0);
            overlayRen->AddActor(m_crossActorsV3D[i]);
        }


        double hP1[3], hP2[3], vP1[3], vP2[3];
        if (i == Axial) { // XY
            double z = m_crossPoint[2];
            hP1[0] = worldMin[0]; hP1[1] = m_crossPoint[1]; hP1[2] = z;
            hP2[0] = worldMax[0]; hP2[1] = m_crossPoint[1]; hP2[2] = z;
            vP1[0] = m_crossPoint[0]; vP1[1] = worldMin[1]; vP1[2] = z;
            vP2[0] = m_crossPoint[0]; vP2[1] = worldMax[1]; vP2[2] = z;
        }
        else if (i == Sagittal) { // YZ
            double x = m_crossPoint[0];
            hP1[0] = x; hP1[1] = worldMin[1]; hP1[2] = m_crossPoint[2];
            hP2[0] = x; hP2[1] = worldMax[1]; hP2[2] = m_crossPoint[2];
            vP1[0] = x; vP1[1] = m_crossPoint[1]; vP1[2] = worldMin[2];
            vP2[0] = x; vP2[1] = m_crossPoint[1]; vP2[2] = worldMax[2];
        }
        else if (i == Coronal) { // XZ
            double y = m_crossPoint[1];
            hP1[0] = worldMin[0]; hP1[1] = y; hP1[2] = m_crossPoint[2];
            hP2[0] = worldMax[0]; hP2[1] = y; hP2[2] = m_crossPoint[2];
            vP1[0] = m_crossPoint[0]; vP1[1] = y; vP1[2] = worldMin[2];
            vP2[0] = m_crossPoint[0]; vP2[1] = y; vP2[2] = worldMax[2];
        }

        qDebug() << "Horizontal Line P1:" << hP1[0] << hP1[1] << hP1[2];
        qDebug() << "Horizontal Line P2:" << hP2[0] << hP2[1] << hP2[2];
        qDebug() << "Vertical Line P1:" << vP1[0] << vP1[1] << vP1[2];
        qDebug() << "Vertical Line P2:" << vP2[0] << vP2[1] << vP2[2];

        m_crossLinesH3D[i]->SetPoint1(hP1);
        m_crossLinesH3D[i]->SetPoint2(hP2);
        m_crossLinesH3D[i]->Modified();

        m_crossLinesV3D[i]->SetPoint1(vP1);
        m_crossLinesV3D[i]->SetPoint2(vP2);
        m_crossLinesV3D[i]->Modified();

        viewer->Render();
    }
}


void ThreeViewController::SetWindowLevel(double ww, double wl) {
    m_windowWidth = ww;
    m_windowLevel = wl;

    for (int i = 0; i < 3; ++i) {
        auto viewer = m_renderers[i]->GetViewer();
        if (!viewer) continue;
        viewer->SetColorWindow(ww);
        viewer->SetColorLevel(wl);

        // 每个视图独立的 TextActor
        if (!m_textActors[i]) {
            m_textActors[i] = vtkSmartPointer<vtkTextActor>::New();
            m_textActors[i]->GetTextProperty()->SetColor(1.0, 1.0, 0.0);
            m_textActors[i]->SetDisplayPosition(10, 10);
            auto prop = m_textActors[i]->GetTextProperty();
            prop->SetFontSize(12);
            viewer->GetRenderer()->AddActor2D(m_textActors[i]);
        }
        m_textActors[i]->SetInput(QString("WW:%1 WL:%2").arg(ww).arg(wl).toStdString().c_str());
        viewer->Render();
    }
}


void ThreeViewController::syncCameras() {
    auto refCam = m_renderers[Axial]->GetViewer()->GetRenderer()->GetActiveCamera();
    double scale = refCam->GetParallelScale();

    for (int i = 0; i < 3; ++i) {
        auto cam = m_renderers[i]->GetViewer()->GetRenderer()->GetActiveCamera();
        cam->SetParallelScale(scale);
        m_renderers[i]->GetViewer()->GetRenderer()->ResetCameraClippingRange();
    }
}