#include "SimpleContourOverlayManager.h"

#include <vtkRenderer.h>
#include <vtkImageViewer2.h>
#include <vtkImageData.h>
#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyLine.h>
#include <vtkProperty.h>
#include <vtkNew.h>

#include <cmath>
#include <limits>
#include <QDebug>

SimpleContourOverlayManager::SimpleContourOverlayManager() = default;
SimpleContourOverlayManager::~SimpleContourOverlayManager() { Shutdown(); }

void SimpleContourOverlayManager::Initialize(vtkRenderer* renderer)
{
    if (m_initialized || !renderer) return;
    m_overlayRenderer = renderer;
    m_initialized = true;
    if (!m_rois.empty() && m_records.empty()) BuildActors();
}

void SimpleContourOverlayManager::Shutdown()
{
    if (!m_initialized) return;
    RemoveAllActors();
    m_rois.clear();
    m_overlayRenderer = nullptr;
    m_initialized = false;
}

void SimpleContourOverlayManager::SetVisible(bool visible)
{
    m_visible = visible;
    for (auto& rec : m_records)
        if (rec.actor) rec.actor->SetVisibility(false);
}

void SimpleContourOverlayManager::SetColor(double, double, double) {}

void SimpleContourOverlayManager::OnSliceChanged(vtkImageViewer2* viewer,
    int slice, ViewType viewType)
{
    if (!m_initialized || m_records.empty()) return;
    if (!viewer || !viewer->GetInput()) return;

    double sliceZ = GetSliceWorldZ(viewer, slice, viewType);
    UpdateVisibleContours(sliceZ, viewType);
}

void SimpleContourOverlayManager::SetRois(const std::vector<RtRoi>& rois)
{
    RemoveAllActors();
    m_rois = rois;
    m_records.clear();
    if (m_initialized) BuildActors();
}

void SimpleContourOverlayManager::ClearAll()
{
    RemoveAllActors();
    m_rois.clear();
    m_records.clear();
}

// ============================================================
//  预建所有轮廓 Actor（初始全隐藏）
//
//  RT-S 的轮廓坐标已经是世界坐标（LPS，mm）。
//  由于 CTViewer 用 vtkImageReslice 做了重采样（方向矩阵对齐），
//  VTK 世界坐标与 DICOM LPS 坐标对齐，可以直接使用。
//
//  每条轮廓对应一条切片上的闭合折线，sliceZ 用第三个坐标（Z轴）标记。
// ============================================================

void SimpleContourOverlayManager::BuildActors()
{
    if (!m_overlayRenderer) return;

    for (const auto& roi : m_rois) {
        for (const auto& contour : roi.contours) {
            if (contour.points.size() < 2) continue;

            // 用轮廓点的 Z 值作为该切片的世界 Z 坐标
            // CreateContourActor 会把所有点的 Z 强制对齐到 sliceZ，
            // 消除轮廓与切片平面之间的微小深度偏差（防止被 CT 图像遮挡）
            auto actor = CreateContourActor(contour.points, roi.color, contour.sliceZ);
            if (!actor) continue;

            actor->SetVisibility(true);
            m_overlayRenderer->AddActor(actor);

            ContourRecord rec;
            rec.actor = actor;
            rec.sliceZ = contour.sliceZ;
            rec.viewType = ViewType::Axial;
            m_records.push_back(rec);
        }
    }

    qDebug() << "[ContourOverlay] 共建立" << m_records.size() << "条轮廓 Actor";
}

// ============================================================
//  创建闭合折线 Actor
//
//  关键：把所有点的 Z 坐标强制对齐到 sliceZ。
//  原因：
//    vtkImageViewer2 的 Axial 切片渲染器开启了深度测试，
//    如果轮廓点 Z 与切片图像 Z 有哪怕 0.001mm 的偏差，
//    轮廓就会被切片图像遮挡而不可见。
//    强制对齐后轮廓与图像共面（或微小正偏移），确保可见。
// ============================================================

vtkSmartPointer<vtkActor> SimpleContourOverlayManager::CreateContourActor(
    const std::vector<std::array<double, 3>>& pts,
    const std::array<double, 3>& color,
    double                                    sliceZ)
{
    const vtkIdType n = static_cast<vtkIdType>(pts.size());
    if (n < 2) return nullptr;

    auto vtkPts = vtkSmartPointer<vtkPoints>::New();
    vtkPts->SetNumberOfPoints(n);

    for (vtkIdType i = 0; i < n; ++i) {
        // 强制 Z 对齐：使用 sliceZ 而不是原始点的 Z 值
        // 注意：这里假设 Axial 视图，Z 轴是切片方向
        // 如需支持 Sagittal/Coronal，需要按视图选择对应轴
        double p[3] = { pts[i][0], pts[i][1], sliceZ };
        vtkPts->SetPoint(i, p);
    }

    // 闭合折线
    auto polyLine = vtkSmartPointer<vtkPolyLine>::New();
    polyLine->GetPointIds()->SetNumberOfIds(n + 1);
    for (vtkIdType i = 0; i < n; ++i)
        polyLine->GetPointIds()->SetId(i, i);
    polyLine->GetPointIds()->SetId(n, 0);   // 闭合

    auto cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(polyLine);

    auto pd = vtkSmartPointer<vtkPolyData>::New();
    pd->SetPoints(vtkPts);
    pd->SetLines(cells);

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(pd);

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(color[0], color[1], color[2]);
    actor->GetProperty()->SetLineWidth(2.0f);
    actor->GetProperty()->BackfaceCullingOff();

    return actor;
}

void SimpleContourOverlayManager::UpdateVisibleContours(double sliceZ, ViewType viewType)
{
    if (!m_visible) return;

    // 只在 Axial 视图显示（RT-S 轮廓坐标与 Axial Z 对应）
    if (viewType != ViewType::Axial) {
        for (auto& rec : m_records)
            if (rec.actor) rec.actor->SetVisibility(false);
        return;
    }

    for (auto& rec : m_records) {
        if (!rec.actor) continue;
        bool match = (std::fabs(rec.sliceZ - sliceZ) <= m_tolerance);
        rec.actor->SetVisibility(match ? 1 : 0);
    }
}

double SimpleContourOverlayManager::GetSliceWorldZ(vtkImageViewer2* viewer,
    int slice,
    ViewType viewType) const
{
    if (!viewer || !viewer->GetInput()) return 0.0;

    double spacing[3], origin[3];
    viewer->GetInput()->GetSpacing(spacing);
    viewer->GetInput()->GetOrigin(origin);

    switch (viewType) {
    case ViewType::Axial:
        return origin[2] + slice * spacing[2];
    default:
        return std::numeric_limits<double>::max();
    }
}

void SimpleContourOverlayManager::RemoveAllActors()
{
    if (!m_overlayRenderer) return;
    for (auto& rec : m_records)
        if (rec.actor) m_overlayRenderer->RemoveActor(rec.actor);
    m_records.clear();
}
