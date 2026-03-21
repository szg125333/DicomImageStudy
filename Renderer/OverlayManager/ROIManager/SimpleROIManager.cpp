#include "SimpleROIManager.h"

#include <vtkRenderer.h>
#include <vtkImageViewer2.h>
#include <vtkImageData.h>
#include <vtkCamera.h>
#include <vtkActor.h>
#include <vtkFollower.h>
#include <vtkLineSource.h>
#include <vtkAppendPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkPolygon.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkVectorText.h>
#include <vtkMath.h>
#include <vtkNew.h>

#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <cassert>

// ============================================================
//  构造 / 析构
// ============================================================

SimpleROIManager::SimpleROIManager() = default;
SimpleROIManager::~SimpleROIManager() { Shutdown(); }

// ============================================================
//  IOverlayFeature —— 生命周期
// ============================================================

void SimpleROIManager::Initialize(vtkRenderer* overlayRenderer)
{
    if (m_initialized || !overlayRenderer) return;
    m_overlayRenderer = overlayRenderer;
    m_initialized = true;
}

void SimpleROIManager::Shutdown()
{
    if (!m_initialized) return;
    ClearAllROI();
    m_overlayRenderer = nullptr;
    m_initialized = false;
}

// ============================================================
//  IOverlayFeature —— 样式
// ============================================================

void SimpleROIManager::SetVisible(bool visible)
{
    m_visible = visible;
    if (!m_initialized) return;

    // 控制所有已完成 ROI 的可见性
    for (auto& [id, roi] : m_rois) {
        if (roi.borderActor)  roi.borderActor->SetVisibility(visible);
        if (roi.fillActor)    roi.fillActor->SetVisibility(visible);
        if (roi.labelFollower)roi.labelFollower->SetVisibility(visible);
    }
    // 预览框
    for (auto& pl : m_previewLines) {
        if (pl.actor) pl.actor->SetVisibility(visible && m_isDrawing);
    }
}

void SimpleROIManager::SetColor(double /*r*/, double /*g*/, double /*b*/)
{
    // 预留，当前颜色在创建 Actor 时硬编码（黄色框 / 蓝色填充）
}

// ============================================================
//  IOverlayFeature —— 切片变化
// ============================================================

void SimpleROIManager::OnSliceChanged(vtkImageViewer2* viewer,
    int              slice,
    ViewType         viewType)
{
    if (!m_initialized || !viewer || !viewer->GetInput()) return;

    double spacing[3], origin[3];
    viewer->GetInput()->GetSpacing(spacing);
    viewer->GetInput()->GetOrigin(origin);

    // 将所有已完成 ROI 的坐标投影到新切片平面
    for (auto& [id, roi] : m_rois) {
        if (!roi.isComplete || roi.viewType != viewType) continue;

        // 只更新与本视图法线方向对应的坐标分量
        switch (viewType) {
        case ViewType::Axial: {
            double newZ = origin[2] + slice * spacing[2];
            roi.corner1[2] = newZ;
            roi.corner2[2] = newZ;
            break;
        }
        case ViewType::Sagittal: {
            double newX = origin[0] + slice * spacing[0];
            roi.corner1[0] = newX;
            roi.corner2[0] = newX;
            break;
        }
        case ViewType::Coronal: {
            double newY = origin[1] + slice * spacing[1];
            roi.corner1[1] = newY;
            roi.corner2[1] = newY;
            break;
        }
        default:
            continue;
        }

        roi.slice = slice;

        // 重绘边框和填充，并重算统计
        RemoveRoiActors(roi);

        std::array<double, 3> corners[4];
        ComputeRectCorners(roi.corner1, roi.corner2, roi.viewType, corners);

        roi.borderActor = CreateBorderActor(corners, false);
        roi.fillActor = CreateFillActor(corners);
        roi.stats = ComputeStats(viewer, roi.corner1, roi.corner2,
            roi.viewType, slice);

        // 标签锚点取矩形右上角
        auto cam = m_overlayRenderer->GetActiveCamera();
        vtkNew<vtkVectorText> textSrc;
        roi.labelFollower = CreateStatsLabel(roi.stats, corners[1], cam);

        m_overlayRenderer->AddActor(roi.borderActor);
        m_overlayRenderer->AddActor(roi.fillActor);
        if (roi.labelFollower) m_overlayRenderer->AddViewProp(roi.labelFollower);
    }
}

// ============================================================
//  绘制接口
// ============================================================

void SimpleROIManager::BeginROI(const std::array<double, 3>& cornerWorld)
{
    if (!m_initialized) return;

    m_isDrawing = true;
    m_drawCorner1 = cornerWorld;
    // viewType 和 slice 由 Strategy 在 CommitROI 时传入
}

void SimpleROIManager::UpdatePreview(const std::array<double, 3>& oppositeCornerWorld)
{
    if (!m_initialized || !m_isDrawing) return;

    std::array<double, 3> corners[4];
    ComputeRectCorners(m_drawCorner1, oppositeCornerWorld, m_drawView, corners);
    UpdatePreviewActors(corners);
}

void SimpleROIManager::CommitROI(const std::array<double, 3>& oppositeCornerWorld,
    vtkImageViewer2* viewer,
    ViewType                      viewType,
    int                           slice)
{
    if (!m_initialized || !m_isDrawing) return;

    // 隐藏预览框
    for (auto& pl : m_previewLines) {
        if (pl.actor) pl.actor->SetVisibility(false);
    }
    if (m_previewLabelFollower) m_previewLabelFollower->SetVisibility(false);

    m_isDrawing = false;

    // 对角点太近（拖拽距离过小）时忽略
    const double dx = oppositeCornerWorld[0] - m_drawCorner1[0];
    const double dy = oppositeCornerWorld[1] - m_drawCorner1[1];
    const double dz = oppositeCornerWorld[2] - m_drawCorner1[2];
    if (std::sqrt(dx * dx + dy * dy + dz * dz) < 1.0) return;

    // 创建 ROI 记录
    int id = NextId();
    m_lastId = id;

    RoiRecord roi;
    roi.id = id;
    roi.corner1 = m_drawCorner1;
    roi.corner2 = oppositeCornerWorld;
    roi.viewType = viewType;
    roi.slice = slice;
    roi.isComplete = true;

    // 计算统计
    roi.stats = ComputeStats(viewer, roi.corner1, roi.corner2, viewType, slice);

    // 计算 4 个角点并创建 Actor
    std::array<double, 3> corners[4];
    ComputeRectCorners(roi.corner1, roi.corner2, viewType, corners);

    roi.borderActor = CreateBorderActor(corners, false);
    roi.fillActor = CreateFillActor(corners);

    auto* cam = m_overlayRenderer->GetActiveCamera();
    roi.labelFollower = CreateStatsLabel(roi.stats, corners[1], cam);

    m_overlayRenderer->AddActor(roi.borderActor);
    m_overlayRenderer->AddActor(roi.fillActor);
    if (roi.labelFollower) m_overlayRenderer->AddViewProp(roi.labelFollower);

    m_rois.emplace(id, std::move(roi));
}

void SimpleROIManager::CancelCurrentROI()
{
    if (!m_isDrawing) return;

    for (auto& pl : m_previewLines) {
        if (pl.actor) pl.actor->SetVisibility(false);
    }
    if (m_previewLabelFollower) m_previewLabelFollower->SetVisibility(false);

    m_isDrawing = false;
}

void SimpleROIManager::DeleteLastROI()
{
    if (m_lastId < 0) return;

    auto it = m_rois.find(m_lastId);
    if (it == m_rois.end()) return;

    RemoveRoiActors(it->second);
    m_rois.erase(it);

    // 更新 m_lastId 到上一个 ROI
    m_lastId = m_rois.empty() ? -1 : m_rois.end()->first;
}

void SimpleROIManager::ClearAllROI()
{
    for (auto& [id, roi] : m_rois) {
        RemoveRoiActors(roi);
    }
    m_rois.clear();
    m_lastId = -1;

    // 同时清除预览
    for (auto& pl : m_previewLines) {
        if (pl.actor && m_overlayRenderer) {
            m_overlayRenderer->RemoveActor(pl.actor);
        }
        pl.source = nullptr;
        pl.mapper = nullptr;
        pl.actor = nullptr;
    }
    if (m_previewLabelFollower && m_overlayRenderer) {
        m_overlayRenderer->RemoveViewProp(m_previewLabelFollower);
        m_previewLabelFollower = nullptr;
        m_previewLabelText = nullptr;
    }
    m_previewInitialized = false;
    m_isDrawing = false;
}

// ============================================================
//  查询接口
// ============================================================

RoiStats SimpleROIManager::GetLastStats() const
{
    if (m_lastId < 0) return {};
    auto it = m_rois.find(m_lastId);
    return it != m_rois.end() ? it->second.stats : RoiStats{};
}

RoiStats SimpleROIManager::GetStats(int roiId) const
{
    auto it = m_rois.find(roiId);
    return it != m_rois.end() ? it->second.stats : RoiStats{};
}

// ============================================================
//  私有方法 —— 坐标计算
// ============================================================

/**
 * 根据两个对角点和视图方向，推导矩形 4 个角点（顺序：左下→右下→右上→左上）。
 *
 * 视图方向决定矩形所在平面：
 *   Axial    → XY 平面，Z 固定
 *   Sagittal → YZ 平面，X 固定
 *   Coronal  → XZ 平面，Y 固定
 */
void SimpleROIManager::ComputeRectCorners(const std::array<double, 3>& c1,
    const std::array<double, 3>& c2,
    ViewType viewType,
    std::array<double, 3> outCorners[4]) const
{
    // 根据视图确定两个自由轴（矩形在这两个轴上延伸）和固定轴
    int axis0 = 0, axis1 = 1;   // 矩形平面上的两个自由轴
    int axisFixed = 2;           // 法线方向的固定轴

    switch (viewType) {
    case ViewType::Axial:    axis0 = 0; axis1 = 1; axisFixed = 2; break;
    case ViewType::Sagittal: axis0 = 1; axis1 = 2; axisFixed = 0; break;
    case ViewType::Coronal:  axis0 = 0; axis1 = 2; axisFixed = 1; break;
    default:
        axis0 = 0; axis1 = 1; axisFixed = 2; break;
    }

    double fixedVal = c1[axisFixed];

    double minA = std::min(c1[axis0], c2[axis0]);
    double maxA = std::max(c1[axis0], c2[axis0]);
    double minB = std::min(c1[axis1], c2[axis1]);
    double maxB = std::max(c1[axis1], c2[axis1]);

    // 4 个角点（顺时针：左下 → 右下 → 右上 → 左上）
    outCorners[0][axis0] = minA; outCorners[0][axis1] = minB; outCorners[0][axisFixed] = fixedVal;
    outCorners[1][axis0] = maxA; outCorners[1][axis1] = minB; outCorners[1][axisFixed] = fixedVal;
    outCorners[2][axis0] = maxA; outCorners[2][axis1] = maxB; outCorners[2][axisFixed] = fixedVal;
    outCorners[3][axis0] = minA; outCorners[3][axis1] = maxB; outCorners[3][axisFixed] = fixedVal;
}

// ============================================================
//  私有方法 —— 预览 Actor
// ============================================================

void SimpleROIManager::UpdatePreviewActors(const std::array<double, 3> corners[4])
{
    if (!m_overlayRenderer) return;

    // 懒初始化：4 条线段（边框）
    if (!m_previewInitialized) {
        for (int i = 0; i < 4; ++i) {
            m_previewLines[i].source = vtkSmartPointer<vtkLineSource>::New();
            m_previewLines[i].mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            m_previewLines[i].mapper->SetInputConnection(
                m_previewLines[i].source->GetOutputPort());
            m_previewLines[i].actor = vtkSmartPointer<vtkActor>::New();
            m_previewLines[i].actor->SetMapper(m_previewLines[i].mapper);
            m_previewLines[i].actor->GetProperty()->SetColor(1.0, 1.0, 0.0);  // 黄色
            m_previewLines[i].actor->GetProperty()->SetLineWidth(1.5f);
            m_previewLines[i].actor->GetProperty()->SetLineStipplePattern(0xF0F0); // 虚线
            m_overlayRenderer->AddActor(m_previewLines[i].actor);
        }
        m_previewInitialized = true;
    }

    // 4 条边：0→1, 1→2, 2→3, 3→0
    for (int i = 0; i < 4; ++i) {
        const auto& p1 = corners[i];
        const auto& p2 = corners[(i + 1) % 4];
        m_previewLines[i].source->SetPoint1(p1.data());
        m_previewLines[i].source->SetPoint2(p2.data());
        m_previewLines[i].source->Modified();
        m_previewLines[i].actor->SetVisibility(true);
    }

    // 懒初始化预览统计标签（使用空文字占位，CommitROI 前不显示统计）
    if (!m_previewLabelFollower) {
        m_previewLabelText = vtkSmartPointer<vtkVectorText>::New();
        m_previewLabelText->SetText("...");

        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(m_previewLabelText->GetOutputPort());

        m_previewLabelFollower = vtkSmartPointer<vtkFollower>::New();
        m_previewLabelFollower->SetMapper(mapper);
        m_previewLabelFollower->SetScale(7.0, 7.0, 7.0);
        m_previewLabelFollower->SetCamera(m_overlayRenderer->GetActiveCamera());
        m_previewLabelFollower->GetProperty()->SetColor(1.0, 1.0, 0.0);
        m_overlayRenderer->AddViewProp(m_previewLabelFollower);
    }

    // 标签跟随矩形右上角（corners[2]）
    m_previewLabelFollower->SetPosition(
        corners[2][0], corners[2][1], corners[2][2]);
    m_previewLabelFollower->SetVisibility(true);
}

// ============================================================
//  私有方法 —— Actor 工厂
// ============================================================

vtkSmartPointer<vtkActor> SimpleROIManager::CreateBorderActor(
    const std::array<double, 3> corners[4], bool /*isPreview*/)
{
    // 将 4 条边合并为一个 PolyData
    auto append = vtkSmartPointer<vtkAppendPolyData>::New();

    for (int i = 0; i < 4; ++i) {
        auto line = vtkSmartPointer<vtkLineSource>::New();
        line->SetPoint1(corners[i].data());
        line->SetPoint2(corners[(i + 1) % 4].data());
        line->Update();
        append->AddInputData(line->GetOutput());
    }
    append->Update();

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(append->GetOutput());

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1.0, 1.0, 0.0);  // 黄色实线
    actor->GetProperty()->SetLineWidth(2.0f);
    return actor;
}

vtkSmartPointer<vtkActor> SimpleROIManager::CreateFillActor(
    const std::array<double, 3> corners[4])
{
    // 用 vtkPolygon 构建一个半透明矩形面
    auto points = vtkSmartPointer<vtkPoints>::New();
    points->SetNumberOfPoints(4);
    for (int i = 0; i < 4; ++i) {
        points->SetPoint(i, corners[i].data());
    }

    auto polygon = vtkSmartPointer<vtkPolygon>::New();
    polygon->GetPointIds()->SetNumberOfIds(4);
    for (int i = 0; i < 4; ++i) {
        polygon->GetPointIds()->SetId(i, i);
    }

    auto cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(polygon);

    auto polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    polyData->SetPolys(cells);

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polyData);

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(0.2, 0.4, 1.0);  // 蓝色
    actor->GetProperty()->SetOpacity(0.15);          // 半透明填充
    return actor;
}

vtkSmartPointer<vtkFollower> SimpleROIManager::CreateStatsLabel(
    const RoiStats& stats,
    const std::array<double, 3>& anchorWorld,
    vtkCamera* camera)
{
    if (!camera) return nullptr;

    // 构造标签文字
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "Mean: " << stats.mean << "\n";
    oss << "SD: " << stats.stdDev << "\n";
    oss << "Min: " << stats.minVal << "\n";
    oss << "Max: " << stats.maxVal << "\n";
    oss << "Area: " << stats.area << " mm2\n";
    oss << "Pixels: " << stats.pixelCount;

    // vtkVectorText 不支持 '\n'，用空格布局替代（实际效果取决于 VTK 版本）
    // 这里改为单行紧凑格式，方便显示
    std::ostringstream singleLine;
    singleLine << std::fixed << std::setprecision(1);
    singleLine << "Mn:" << stats.mean
        << " SD:" << stats.stdDev
        << " Mn/Mx:" << stats.minVal << "/" << stats.maxVal
        << " " << stats.area << "mm2";

    auto textSrc = vtkSmartPointer<vtkVectorText>::New();
    textSrc->SetText(singleLine.str().c_str());

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(textSrc->GetOutputPort());

    auto follower = vtkSmartPointer<vtkFollower>::New();
    follower->SetMapper(mapper);
    follower->SetScale(6.0, 6.0, 6.0);
    follower->SetCamera(camera);
    follower->GetProperty()->SetColor(1.0, 1.0, 0.0);  // 黄色文字
    follower->SetPosition(anchorWorld[0], anchorWorld[1], anchorWorld[2]);
    return follower;
}

// ============================================================
//  私有方法 —— 统计计算
// ============================================================

RoiStats SimpleROIManager::ComputeStats(vtkImageViewer2* viewer,
    const std::array<double, 3>& c1,
    const std::array<double, 3>& c2,
    ViewType                      viewType,
    int                           slice) const
{
    RoiStats result;
    if (!viewer || !viewer->GetInput()) return result;

    vtkImageData* imageData = viewer->GetInput();

    double spacing[3], origin[3];
    int    dims[3];
    imageData->GetSpacing(spacing);
    imageData->GetOrigin(origin);
    imageData->GetDimensions(dims);

    // 确定矩形的世界坐标范围（两对角点取 min/max）
    double worldMin[3], worldMax[3];
    for (int i = 0; i < 3; ++i) {
        worldMin[i] = std::min(c1[i], c2[i]);
        worldMax[i] = std::max(c1[i], c2[i]);
    }

    // 世界坐标 → 体素索引（夹紧到有效范围）
    auto worldToIndex = [&](double world, int axis) -> int {
        int idx = static_cast<int>((world - origin[axis]) / spacing[axis] + 0.5);
        idx = std::max(idx, 0);
        idx = std::min(idx, dims[axis] - 1);
        return idx;
        };

    // 根据视图方向确定遍历的两个轴和固定轴
    int ax0 = 0, ax1 = 1, axFixed = 2;
    switch (viewType) {
    case ViewType::Axial:    ax0 = 0; ax1 = 1; axFixed = 2; break;
    case ViewType::Sagittal: ax0 = 1; ax1 = 2; axFixed = 0; break;
    case ViewType::Coronal:  ax0 = 0; ax1 = 2; axFixed = 1; break;
    default: break;
    }

    // 固定轴的体素索引就是当前切片
    int fixedIdx = slice;

    // 遍历范围（两个自由轴）
    int minIdx0 = worldToIndex(worldMin[ax0], ax0);
    int maxIdx0 = worldToIndex(worldMax[ax0], ax0);
    int minIdx1 = worldToIndex(worldMin[ax1], ax1);
    int maxIdx1 = worldToIndex(worldMax[ax1], ax1);

    if (minIdx0 > maxIdx0) std::swap(minIdx0, maxIdx0);
    if (minIdx1 > maxIdx1) std::swap(minIdx1, maxIdx1);

    // 收集所有体素的标量值（HU 值）
    double sum = 0.0;
    double sumSq = 0.0;
    double minV = std::numeric_limits<double>::max();
    double maxV = std::numeric_limits<double>::lowest();
    int    count = 0;

    int ijk[3];
    for (int i = minIdx0; i <= maxIdx0; ++i) {
        for (int j = minIdx1; j <= maxIdx1; ++j) {
            ijk[ax0] = i;
            ijk[ax1] = j;
            ijk[axFixed] = fixedIdx;

            // 越界保护
            if (ijk[0] < 0 || ijk[0] >= dims[0]) continue;
            if (ijk[1] < 0 || ijk[1] >= dims[1]) continue;
            if (ijk[2] < 0 || ijk[2] >= dims[2]) continue;

            double val = imageData->GetScalarComponentAsDouble(
                ijk[0], ijk[1], ijk[2], 0);

            sum += val;
            sumSq += val * val;
            minV = std::min(minV, val);
            maxV = std::max(maxV, val);
            ++count;
        }
    }

    if (count == 0) return result;

    result.pixelCount = count;
    result.mean = sum / count;
    result.minVal = minV;
    result.maxVal = maxV;

    // 标准差：σ = sqrt(E[X²] - E[X]²)
    double variance = (sumSq / count) - (result.mean * result.mean);
    result.stdDev = (variance > 0.0) ? std::sqrt(variance) : 0.0;

    // 面积（mm²）：两个自由轴的像素数 × spacing 之积
    int pixW = maxIdx0 - minIdx0 + 1;
    int pixH = maxIdx1 - minIdx1 + 1;
    result.area = pixW * spacing[ax0] * pixH * spacing[ax1];

    return result;
}

// ============================================================
//  私有方法 —— Actor 移除
// ============================================================

void SimpleROIManager::RemoveRoiActors(RoiRecord& roi)
{
    if (!m_overlayRenderer) return;

    auto removeIfValid = [this](vtkSmartPointer<vtkProp> prop) {
        if (prop) m_overlayRenderer->RemoveViewProp(prop);
        };

    removeIfValid(roi.borderActor);
    removeIfValid(roi.fillActor);
    removeIfValid(roi.labelFollower);

    roi.borderActor = nullptr;
    roi.fillActor = nullptr;
    roi.labelFollower = nullptr;
}