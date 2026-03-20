#include "SimpleAngleMeasureManager.h"
#include <vtkRenderer.h>
#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkLine.h>
#include <vtkPolyLine.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkPolyDataMapper.h>
#include <vtkImageViewer2.h>
#include <QDebug>
#include <vtkRegularPolygonSource.h>
#include <vtkActor2D.h>
#include <vtkCursor2D.h>
#include <vtkLineSource.h>
#include <vtkAppendPolyData.h>
#include <vtkSphereSource.h>
#include <vtkRenderWindow.h>
#include <vtkMath.h>
#include <vtkPropPicker.h>
#include <vtkVectorText.h>
#include <vtkCoordinate.h>
#include <vtkFollower.h>

constexpr double PI = 3.14159265358979323846;

SimpleAngleMeasureManager::SimpleAngleMeasureManager() = default;

SimpleAngleMeasureManager::~SimpleAngleMeasureManager() {
    Shutdown();
}

void SimpleAngleMeasureManager::Initialize(vtkRenderer* overlayRenderer) {
    if (m_initialized) return;
    if (!overlayRenderer) return;

    m_overlayRenderer = overlayRenderer;
    m_initialized = true;

    qDebug() << "[SimpleAngleMeasureManager] Initialized";
}

void SimpleAngleMeasureManager::SetColor(double r, double g, double b)
{

}

void SimpleAngleMeasureManager::StartMeasure(const std::array<double, 3>& point1) {
    qDebug() << "[SimpleAngleMeasureManager] StartMeasure - Point1:"
        << point1[0] << point1[1] << point1[2];
}

void SimpleAngleMeasureManager::UpdateMeasure(const std::array<double, 3>& point2) {
    qDebug() << "[SimpleAngleMeasureManager] UpdateMeasure - Point2:"
        << point2[0] << point2[1] << point2[2];
}

void SimpleAngleMeasureManager::EndMeasure(const std::array<double, 3>& point3) {
    qDebug() << "[SimpleAngleMeasureManager] EndMeasure - Point3:"
        << point3[0] << point3[1] << point3[2];
}

void SimpleAngleMeasureManager::SetVisible(bool visible) {
    m_visible = visible;
    qDebug() << "[SimpleAngleMeasureManager] SetVisible:" << visible;
}

void SimpleAngleMeasureManager::Shutdown() {
    if (!m_initialized) return;

    qDebug() << "[SimpleAngleMeasureManager] Shutdown";
    m_overlayRenderer = nullptr;
    m_viewer = nullptr;
    m_initialized = false;
}

void SimpleAngleMeasureManager::DrawStartPoint(std::array<double, 3> worldPoint) {
    if (!m_overlayRenderer) return;
    int id = generateNextId();
    auto [it, inserted] = m_measurements.emplace(id, Measurement{});
    assert(inserted);
    Measurement& m = it->second;
    m.id = id;
    m.startPointWorld = worldPoint;
    m.isComplete = false;

    m.startPointActor = createSphereActor(worldPoint);
    m.startCrosshairActor = createCrosshairActor(worldPoint, 5.0);

    m_overlayRenderer->AddActor(m.startPointActor);
    m_overlayRenderer->AddActor(m.startCrosshairActor);
}

void SimpleAngleMeasureManager::PreviewStartToMiddleMeasurementLine(std::array<double, 3> startPos, std::array<double, 3> currentPos)
{
    if (!m_overlayRenderer || !m_initialized) return;
    auto cam = m_overlayRenderer->GetActiveCamera();
    if (!cam) return;

    // === 懒初始化：只创建一次 ===
    if (!m_previewStartToMiddleLineActor) {
        m_previewStartToMiddleLineSource = vtkSmartPointer<vtkLineSource>::New();
        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(m_previewStartToMiddleLineSource->GetOutputPort());
        m_previewStartToMiddleLineActor = vtkSmartPointer<vtkActor>::New();
        m_previewStartToMiddleLineActor->SetMapper(mapper);
        m_previewStartToMiddleLineActor->GetProperty()->SetColor(0.0, 1.0, 0.0);
        m_overlayRenderer->AddViewProp(m_previewStartToMiddleLineActor);
    }
    // === 更新数据（零重建）===
    m_previewStartToMiddleLineSource->SetPoint1(startPos.data());
    m_previewStartToMiddleLineSource->SetPoint2(currentPos.data());
    m_previewStartToMiddleLineSource->Modified();
}

void SimpleAngleMeasureManager::DrawMiddlePointAndStartToMiddleLine(std::array<double, 3> worldPoint)
{
    if (!m_overlayRenderer) return;
    auto it = m_measurements.find(m_currentId);
    Measurement& m = it->second;
    m.middlePointWorld = worldPoint;
	m.startToMiddleLine1Actor = createLineActor(m.startPointWorld, m.middlePointWorld);
    m.middlePointActor = createSphereActor(worldPoint);
    m.middleCrosshairActor = createCrosshairActor(worldPoint, 5.0);

    m_overlayRenderer->AddActor(m.startToMiddleLine1Actor);
    m_overlayRenderer->AddActor(m.middlePointActor);
    m_overlayRenderer->AddActor(m.middleCrosshairActor);
}

void SimpleAngleMeasureManager::PreviewMiddleToEndMeasurementLine(
    std::array<double, 3> startPos,      // 实际是 middlePoint
    std::array<double, 3> currentPos)    // 鼠标当前位置（end point）
{
    if (!m_overlayRenderer || !m_initialized) return;
    auto cam = m_overlayRenderer->GetActiveCamera();
    if (!cam) return;

    // === 1. 更新预览线段 ===
    if (!m_previewMiddleToEndLineActor) {
        m_previewMiddleToEndLineSource = vtkSmartPointer<vtkLineSource>::New();
        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(m_previewMiddleToEndLineSource->GetOutputPort());
        m_previewMiddleToEndLineActor = vtkSmartPointer<vtkActor>::New();
        m_previewMiddleToEndLineActor->SetMapper(mapper);
        m_previewMiddleToEndLineActor->GetProperty()->SetColor(0.0, 1.0, 0.0);
        m_overlayRenderer->AddViewProp(m_previewMiddleToEndLineActor);
    }
    m_previewMiddleToEndLineSource->SetPoint1(startPos.data());
    m_previewMiddleToEndLineSource->SetPoint2(currentPos.data());
    m_previewMiddleToEndLineSource->Modified();

    auto it = m_measurements.find(m_currentId);
    if (it != m_measurements.end()) {
        Measurement& m = it->second;
        // === 2. 计算当前角度 ===
        double angleDeg = ComputeAngle(m.startPointWorld, startPos, currentPos);

        // === 3. 更新预览弧线 ===
        UpdatePreviewAngleArc(m.startPointWorld, startPos, currentPos);

        // === 4. 更新预览标签 ===
        UpdatePreviewAngleLabel(angleDeg, m.startPointWorld, startPos, currentPos, cam);
    }
}

void SimpleAngleMeasureManager::DrawEndPointAndMiddleToEndLine(std::array<double, 3> worldPoint)
{
    if (!m_overlayRenderer) return;

    auto it = m_measurements.find(m_currentId);
    if (it == m_measurements.end()) return;
    Measurement& m = it->second;

    m.endPointWorld = worldPoint;
    m.isComplete = true; // 标记为完成
    // 替换原有绘制逻辑
    //DrawFinalAngleMeasurement(m.id);

    m.middleToEndLineActor = createLineActor(m.middlePointWorld, worldPoint);
    m.endPointActor = createSphereActor(worldPoint);
    m.endCrosshairActor = createCrosshairActor(worldPoint, 5.0);

    m_overlayRenderer->AddActor(m.middleToEndLineActor);
    m_overlayRenderer->AddActor(m.endPointActor);
    m_overlayRenderer->AddActor(m.endCrosshairActor);

    // === 计算角度 ===
    double angleDeg = ComputeAngle(m.startPointWorld, m.middlePointWorld, m.endPointWorld);

    // === 计算标签位置（角内部）===
    std::array<double, 3> labelPos = ComputeAngleLabelPosition(
        m.startPointWorld, m.middlePointWorld, m.endPointWorld, 0.3
    );

    // === 创建可视化元素 ===
    auto cam = m_overlayRenderer->GetActiveCamera();
    if (!cam) return;

    m.angleArcActor = CreateAngleArc(m.startPointWorld, m.middlePointWorld, m.endPointWorld);
    m.angleLabel = CreateAngleLabel(angleDeg, labelPos, cam);

    if (m.angleArcActor) {
        m_overlayRenderer->AddActor(m.angleArcActor);
    }
    if (m.angleLabel) {
        m_overlayRenderer->AddViewProp(m.angleLabel);
    }

    qDebug() << "[Angle] Calculated:" << QString::number(angleDeg, 'f', 1) << "°";

    ClearPreview();
}

vtkSmartPointer<vtkActor> SimpleAngleMeasureManager::createSphereActor(const std::array<double, 3>& point) {
    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetCenter(point.data());
    sphereSource->SetRadius(0.5);
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(sphereSource->GetOutputPort());
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1.0, 0.0, 0.0);
    return actor;
}

vtkSmartPointer<vtkActor> SimpleAngleMeasureManager::createLineActor(const std::array<double, 3>& startPoint, const std::array<double, 3>& endPoint) {
    vtkSmartPointer<vtkLineSource> lineSource = vtkSmartPointer<vtkLineSource>::New();
    lineSource->SetPoint1(startPoint.data());
    lineSource->SetPoint2(endPoint.data());
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(lineSource->GetOutputPort());
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(0.0, 1.0, 0.0);
    return actor;
}

vtkSmartPointer<vtkActor> SimpleAngleMeasureManager::createCrosshairActor(const std::array<double, 3>& center, double length) {
    auto xLine = vtkSmartPointer<vtkLineSource>::New();
    xLine->SetPoint1(center[0] - length, center[1], center[2]);
    xLine->SetPoint2(center[0] + length, center[1], center[2]);
    auto yLine = vtkSmartPointer<vtkLineSource>::New();
    yLine->SetPoint1(center[0], center[1] - length, center[2]);
    yLine->SetPoint2(center[0], center[1] + length, center[2]);
    auto zLine = vtkSmartPointer<vtkLineSource>::New();
    zLine->SetPoint1(center[0], center[1], center[2] - length);
    zLine->SetPoint2(center[0], center[1], center[2] + length);

    auto append = vtkSmartPointer<vtkAppendPolyData>::New();
    append->AddInputConnection(xLine->GetOutputPort());
    append->AddInputConnection(yLine->GetOutputPort());
    append->AddInputConnection(zLine->GetOutputPort());

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(append->GetOutputPort());
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(0.0, 1.0, 1.0);
    actor->GetProperty()->SetLineWidth(2.0);
    return actor;
}

double SimpleAngleMeasureManager::ComputeAngle(
    const std::array<double, 3>& startPoint,
    const std::array<double, 3>& vertexPoint,
    const std::array<double, 3>& endPoint)
{
    // 向量1: start → vertex
    std::array<double, 3> v1 = {
        vertexPoint[0] - startPoint[0],
        vertexPoint[1] - startPoint[1],
        vertexPoint[2] - startPoint[2]
    };
    // 向量2: end → vertex（注意方向！应为 vertex → end）
    std::array<double, 3> v2 = {
        endPoint[0] - vertexPoint[0],
        endPoint[1] - vertexPoint[1],
        endPoint[2] - vertexPoint[2]
    };

    double len1 = vtkMath::Norm(v1.data());
    double len2 = vtkMath::Norm(v2.data());

    if (len1 < 1e-6 || len2 < 1e-6) {
        return 0.0; // 退化情况
    }

    vtkMath::Normalize(v1.data());
    vtkMath::Normalize(v2.data());

    double dot = vtkMath::Dot(v1.data(), v2.data());
    dot = std::clamp(dot, -1.0, 1.0);
    double angleRad = std::acos(dot);
    return vtkMath::DegreesFromRadians(angleRad); // 返回度
}

vtkSmartPointer<vtkActor> SimpleAngleMeasureManager::CreateAngleArc(
    const std::array<double, 3>& startPoint,
    const std::array<double, 3>& vertexPoint,
    const std::array<double, 3>& endPoint)
{
    std::array<double, 3> v1 = {
        startPoint[0] - vertexPoint[0],
        startPoint[1] - vertexPoint[1],
        startPoint[2] - vertexPoint[2]
    };
    std::array<double, 3> v2 = {
        endPoint[0] - vertexPoint[0],
        endPoint[1] - vertexPoint[1],
        endPoint[2] - vertexPoint[2]
    };

    double r1 = vtkMath::Norm(v1.data());
    double r2 = vtkMath::Norm(v2.data());
    if (r1 < 1e-6 || r2 < 1e-6) return nullptr;

    vtkMath::Normalize(v1.data());
    vtkMath::Normalize(v2.data());

    double dot = vtkMath::Dot(v1.data(), v2.data());
    dot = std::clamp(dot, -1.0, 1.0);
    double angle = std::acos(dot);
    if (angle < 1e-3) return nullptr; // 几乎共线

    // 旋转轴
    double axis[3];
    vtkMath::Cross(v1.data(), v2.data(), axis);
    if (vtkMath::Norm(axis) < 1e-6) return nullptr;
    vtkMath::Normalize(axis);

    // 弧半径
    double radius = std::min(r1, r2) * 0.3;
    int nSegments = std::max(5, static_cast<int>(angle * 10));

    vtkNew<vtkPoints> points;
    for (int i = 0; i <= nSegments; ++i) {
        double t = static_cast<double>(i) / nSegments;
        double theta = t * angle;

        // Rodrigues' rotation
        double cosT = std::cos(theta);
        double sinT = std::sin(theta);
        double kx = axis[0], ky = axis[1], kz = axis[2];
        double vx = v1[0], vy = v1[1], vz = v1[2];

        double x = vx * cosT + (ky * vz - kz * vy) * sinT + kx * (kx * vx + ky * vy + kz * vz) * (1 - cosT);
        double y = vy * cosT + (kz * vx - kx * vz) * sinT + ky * (kx * vx + ky * vy + kz * vz) * (1 - cosT);
        double z = vz * cosT + (kx * vy - ky * vx) * sinT + kz * (kx * vx + ky * vy + kz * vz) * (1 - cosT);

        points->InsertNextPoint(
            vertexPoint[0] + radius * x,
            vertexPoint[1] + radius * y,
            vertexPoint[2] + radius * z
        );
    }

    vtkNew<vtkPolyLine> polyLine;
    polyLine->GetPointIds()->SetNumberOfIds(points->GetNumberOfPoints());
    for (vtkIdType i = 0; i < points->GetNumberOfPoints(); ++i) {
        polyLine->GetPointIds()->SetId(i, i);
    }

    vtkNew<vtkCellArray> cells;
    cells->InsertNextCell(polyLine);

    vtkNew<vtkPolyData> polyData;
    polyData->SetPoints(points);
    polyData->SetLines(cells);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(polyData);

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1.0, 0.5, 0.0); // 橙色
    actor->GetProperty()->SetLineWidth(2);

    return actor;
}

vtkSmartPointer<vtkFollower> SimpleAngleMeasureManager::CreateAngleLabel(
    double angleDeg,
    const std::array<double, 3>& labelPosition,
    vtkCamera* camera)
{
    if (!camera) return nullptr;

    QString text = QString::number(angleDeg, 'f', 1) + "°";

    vtkNew<vtkVectorText> textSource;
    textSource->SetText(text.toStdString().c_str());

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(textSource->GetOutputPort());

    vtkNew<vtkFollower> follower;
    follower->SetMapper(mapper);
    follower->SetPosition(
        labelPosition[0],
        labelPosition[1],
        labelPosition[2]
    );
    follower->SetScale(5.0, 5.0, 5.0);
    follower->SetCamera(camera);
    follower->GetProperty()->SetColor(1.0, 1.0, 0.0); // yellow

    return follower;
}

std::array<double, 3> SimpleAngleMeasureManager::ComputeAngleLabelPosition(
    const std::array<double, 3>& start,
    const std::array<double, 3>& vertex,
    const std::array<double, 3>& end,
    double offsetFactor /*= 0.3*/)
{
    // 向量从顶点指向两边
    std::array<double, 3> toStart = { start[0] - vertex[0], start[1] - vertex[1], start[2] - vertex[2] };
    std::array<double, 3> toEnd = { end[0] - vertex[0], end[1] - vertex[1], end[2] - vertex[2] };

    double lenToStart = vtkMath::Norm(toStart.data());
    double lenToEnd = vtkMath::Norm(toEnd.data());

    // 退化情况：任一向量太短
    if (lenToStart < 1e-6 || lenToEnd < 1e-6) {
        auto pos = vertex;
        pos[0] += 10.0;
        return pos;
    }

    // 归一化
    vtkMath::Normalize(toStart.data());
    vtkMath::Normalize(toEnd.data());

    // 角平分线（单位向量之和）
    std::array<double, 3> bisector = {
        toStart[0] + toEnd[0],
        toStart[1] + toEnd[1],
        toStart[2] + toEnd[2]
    };

    double bisLen = vtkMath::Norm(bisector.data());
    if (bisLen > 1e-6) {
        vtkMath::Normalize(bisector.data());
        double offset = std::min(lenToStart, lenToEnd) * offsetFactor;
        std::array<double, 3> pos;
        for (int i = 0; i < 3; ++i) {
            pos[i] = vertex[i] + bisector[i] * offset;
        }
        return pos;
    }
    else {
        // 180° 平角：取垂直方向
        std::array<double, 3> perp;
        if (std::abs(toStart[0]) > 0.1 || std::abs(toStart[1]) > 0.1) {
            perp = { -toStart[1], toStart[0], 0.0 };
        }
        else {
            perp = { 0.0, -toStart[2], toStart[1] };
        }
        vtkMath::Normalize(perp.data());
        std::array<double, 3> pos;
        for (int i = 0; i < 3; ++i) {
            pos[i] = vertex[i] + perp[i] * 10.0;
        }
        return pos;
    }
}

void SimpleAngleMeasureManager::UpdatePreviewAngleArc(
    const std::array<double, 3>& start,
    const std::array<double, 3>& vertex,
    const std::array<double, 3>& end)
{
    // 懒初始化
    if (!m_previewAngleArcActor) {
        // 创建空 polydata 容器（后续只更新点）
        m_previewArcPoints = vtkSmartPointer<vtkPoints>::New();
        m_previewArcPolyLine = vtkSmartPointer<vtkPolyLine>::New();
        m_previewArcCells = vtkSmartPointer<vtkCellArray>::New();
        m_previewArcPolyData = vtkSmartPointer<vtkPolyData>::New();

        m_previewArcPolyLine->GetPointIds()->SetNumberOfIds(0);
        m_previewArcCells->InsertNextCell(m_previewArcPolyLine);
        m_previewArcPolyData->SetPoints(m_previewArcPoints);
        m_previewArcPolyData->SetLines(m_previewArcCells);

        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputData(m_previewArcPolyData);

        m_previewAngleArcActor = vtkSmartPointer<vtkActor>::New();
        m_previewAngleArcActor->SetMapper(mapper);
        m_previewAngleArcActor->GetProperty()->SetColor(1.0, 0.5, 0.0); // orange
        m_previewAngleArcActor->GetProperty()->SetLineWidth(2);
        m_overlayRenderer->AddActor(m_previewAngleArcActor); // ✅ 正确
    }

    // 重新生成弧线点（复用 CreateAngleArc 的核心逻辑，但不新建 Actor）
    GenerateArcPoints(start, vertex, end, m_previewArcPoints, m_previewArcPolyLine);

    auto cells = vtkSmartPointer<vtkCellArray>::New();
    if (m_previewArcPoints->GetNumberOfPoints() > 1) {
        cells->InsertNextCell(m_previewArcPolyLine);
    }

    m_previewArcPolyData->SetPoints(m_previewArcPoints);
    m_previewArcPolyData->SetLines(cells);
    m_previewArcPolyData->Modified();
}

void SimpleAngleMeasureManager::UpdatePreviewAngleLabel(
    double angleDeg,
    const std::array<double, 3>& start,
    const std::array<double, 3>& vertex,
    const std::array<double, 3>& end,
    vtkCamera* camera)
{
    // 懒初始化
    if (!m_previewAngleLabel) {
        m_previewLabelText = vtkSmartPointer<vtkVectorText>::New();
        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(m_previewLabelText->GetOutputPort());

        m_previewAngleLabel = vtkSmartPointer<vtkFollower>::New();
        m_previewAngleLabel->SetMapper(mapper);
        m_previewAngleLabel->SetScale(5.0, 5.0, 5.0);
        m_previewAngleLabel->SetCamera(camera);
        m_previewAngleLabel->GetProperty()->SetColor(1.0, 1.0, 0.0); // yellow
        m_overlayRenderer->AddViewProp(m_previewAngleLabel);
    }

    // 更新文字内容
    QString text = QString::number(angleDeg, 'f', 1) + "°";
    m_previewLabelText->SetText(text.toStdString().c_str());
    m_previewLabelText->Modified(); // 触发更新

    // 更新位置
    std::array<double, 3> labelPos = ComputeAngleLabelPosition(start, vertex, end, 0.3);
    m_previewAngleLabel->SetPosition(labelPos[0], labelPos[1], labelPos[2]);
}

void SimpleAngleMeasureManager::GenerateArcPoints(
    const std::array<double, 3>& startPoint,
    const std::array<double, 3>& vertexPoint,
    const std::array<double, 3>& endPoint,
    vtkPoints* points,
    vtkPolyLine* polyLine)
{
    if (!points || !polyLine) return;

    // 清空旧数据
    points->Reset();
    polyLine->GetPointIds()->SetNumberOfIds(0);

    // 计算向量（从顶点出发）
    std::array<double, 3> v1 = {
        startPoint[0] - vertexPoint[0],
        startPoint[1] - vertexPoint[1],
        startPoint[2] - vertexPoint[2]
    };
    std::array<double, 3> v2 = {
        endPoint[0] - vertexPoint[0],
        endPoint[1] - vertexPoint[1],
        endPoint[2] - vertexPoint[2]
    };

    double r1 = vtkMath::Norm(v1.data());
    double r2 = vtkMath::Norm(v2.data());
    if (r1 < 1e-6 || r2 < 1e-6) return;

    vtkMath::Normalize(v1.data());
    vtkMath::Normalize(v2.data());

    double dot = vtkMath::Dot(v1.data(), v2.data());
    dot = std::clamp(dot, -1.0, 1.0);
    double angle = std::acos(dot);
    if (angle < 1e-3) return;

    // 旋转轴
    double axis[3];
    vtkMath::Cross(v1.data(), v2.data(), axis);
    if (vtkMath::Norm(axis) < 1e-6) return;
    vtkMath::Normalize(axis);

    // 弧半径 & 分段数
    double radius = std::min(r1, r2) * 0.3;
    int nSegments = std::max(5, static_cast<int>(angle * 10));

    // 生成点
    for (int i = 0; i <= nSegments; ++i) {
        double t = static_cast<double>(i) / nSegments;
        double theta = t * angle;

        // Rodrigues' rotation formula
        double cosT = std::cos(theta);
        double sinT = std::sin(theta);
        double kx = axis[0], ky = axis[1], kz = axis[2];
        double vx = v1[0], vy = v1[1], vz = v1[2];

        double x = vx * cosT + (ky * vz - kz * vy) * sinT + kx * (kx * vx + ky * vy + kz * vz) * (1 - cosT);
        double y = vy * cosT + (kz * vx - kx * vz) * sinT + ky * (kx * vx + ky * vy + kz * vz) * (1 - cosT);
        double z = vz * cosT + (kx * vy - ky * vx) * sinT + kz * (kx * vx + ky * vy + kz * vz) * (1 - cosT);

        points->InsertNextPoint(
            vertexPoint[0] + radius * x,
            vertexPoint[1] + radius * y,
            vertexPoint[2] + radius * z
        );
    }

    // 更新 polyLine 连接
    vtkIdType numPoints = points->GetNumberOfPoints();
    polyLine->GetPointIds()->SetNumberOfIds(numPoints);
    for (vtkIdType i = 0; i < numPoints; ++i) {
        polyLine->GetPointIds()->SetId(i, i);
    }
}

void SimpleAngleMeasureManager::ClearPreview()
{
    if (!m_overlayRenderer || !m_initialized) return;

    // 移除预览线段（start → middle）
    if (m_previewStartToMiddleLineActor) {
        m_overlayRenderer->RemoveViewProp(m_previewStartToMiddleLineActor);
        m_previewStartToMiddleLineActor = nullptr;
        m_previewStartToMiddleLineSource = nullptr;
    }

    // 移除预览线段（middle → end）
    if (m_previewMiddleToEndLineActor) {
        m_overlayRenderer->RemoveViewProp(m_previewMiddleToEndLineActor);
        m_previewMiddleToEndLineActor = nullptr;
        m_previewMiddleToEndLineSource = nullptr;
    }

    // 移除预览弧线
    if (m_previewAngleArcActor) {
        m_overlayRenderer->RemoveActor(m_previewAngleArcActor);
        m_previewAngleArcActor = nullptr;
        m_previewArcPoints = nullptr;
        m_previewArcPolyLine = nullptr;
        m_previewArcCells = nullptr;
        m_previewArcPolyData = nullptr;
    }

    // 移除预览标签
    if (m_previewAngleLabel) {
        m_overlayRenderer->RemoveViewProp(m_previewAngleLabel);
        m_previewAngleLabel = nullptr;
        m_previewLabelText = nullptr;
    }
}

void SimpleAngleMeasureManager::ClearCurrentMeasurement()
{
    if (!m_overlayRenderer || m_measurements.empty()) return;

	ClearPreview(); // 先清除预览元素

    int currentId = m_currentId;
    auto it = m_measurements.find(currentId);
    if (it == m_measurements.end()) return;

    Measurement& m = it->second;

    // 安全移除所有 actor
    auto removeIfNotNull = [this](vtkSmartPointer<vtkProp> actor) {
        if (actor) {
            m_overlayRenderer->RemoveActor(actor);
        }
        };

    auto removeViewPropIfNotNull = [this](vtkSmartPointer<vtkProp> prop) {
        if (prop) {
            m_overlayRenderer->RemoveViewProp(prop);
        }
        };

    removeIfNotNull(m.startPointActor);
    removeIfNotNull(m.startCrosshairActor);
    removeIfNotNull(m.middlePointActor);
    removeIfNotNull(m.middleCrosshairActor);
    removeIfNotNull(m.endPointActor);
    removeIfNotNull(m.endCrosshairActor);
    removeIfNotNull(m.startToMiddleLine1Actor);
    removeIfNotNull(m.middleToEndLineActor);
    removeIfNotNull(m.angleArcActor);
    removeViewPropIfNotNull(m.angleLabel);

    // 从 map 中删除
    m_measurements.erase(it);
}

void SimpleAngleMeasureManager::ClearAllMeasurement()
{
    if (!m_overlayRenderer || m_measurements.empty()) return;

	ClearPreview(); // 先清除预览元素

    for (auto& pair : m_measurements) {
        Measurement& m = pair.second;

        auto removeIfNotNull = [this](vtkSmartPointer<vtkProp> actor) {
            if (actor) {
                m_overlayRenderer->RemoveActor(actor);
            }
            };

        auto removeViewPropIfNotNull = [this](vtkSmartPointer<vtkProp> prop) {
            if (prop) {
                m_overlayRenderer->RemoveViewProp(prop);
            }
            };

        removeIfNotNull(m.startPointActor);
        removeIfNotNull(m.startCrosshairActor);
        removeIfNotNull(m.middlePointActor);
        removeIfNotNull(m.middleCrosshairActor);
        removeIfNotNull(m.endPointActor);
        removeIfNotNull(m.endCrosshairActor);
        removeIfNotNull(m.startToMiddleLine1Actor);
        removeIfNotNull(m.middleToEndLineActor);
        removeIfNotNull(m.angleArcActor);
        removeViewPropIfNotNull(m.angleLabel);
    }

    m_measurements.clear();
}

EditableAnglePoint SimpleAngleMeasureManager::GetEditableAnglePoint(int screenX, int screenY) const
{
    if (!m_overlayRenderer) return {};

    // === 第一步：精确拾取（PropPicker）===
    vtkNew<vtkPropPicker> picker;
    if (picker->PickProp(screenX, screenY, m_overlayRenderer)) {
        for (const auto& [id, m] : m_measurements) {
            if (!m.isComplete) continue;
            if (m.startPointActor == picker->GetViewProp()) {
                return { id, AnglePointRole::Start };
            }
            if (m.middlePointActor == picker->GetViewProp()) {
                return { id, AnglePointRole::Middle };
            }
            if (m.endPointActor == picker->GetViewProp()) {
                return { id, AnglePointRole::End };
            }
        }
    }

    // === 第二步：容差拾取（fallback）===
    constexpr double TOLERANCE_PX = 6.0;
    double minDist2 = TOLERANCE_PX * TOLERANCE_PX + 1.0;
    EditableAnglePoint bestMatch{ -1, AnglePointRole::None };

    vtkNew<vtkCoordinate> coord;
    coord->SetCoordinateSystemToWorld();
    coord->SetViewport(m_overlayRenderer);

    for (const auto& [id, m] : m_measurements) {
        if (!m.isComplete) continue;

        // 检查起点
        if (m.startPointActor && m.startPointActor->GetVisibility()) {
            coord->SetValue(m.startPointWorld.data());
            int* dispPos = coord->GetComputedDisplayValue(m_overlayRenderer);
            double dx = dispPos[0] - static_cast<double>(screenX);
            double dy = dispPos[1] - static_cast<double>(screenY);
            double dist2 = dx * dx + dy * dy;
            if (dist2 < minDist2) {
                minDist2 = dist2;
                bestMatch = { id, AnglePointRole::Start };
            }
        }

        // 检查顶点
        if (m.middlePointActor && m.middlePointActor->GetVisibility()) {
            coord->SetValue(m.middlePointWorld.data());
            int* dispPos = coord->GetComputedDisplayValue(m_overlayRenderer);
            double dx = dispPos[0] - static_cast<double>(screenX);
            double dy = dispPos[1] - static_cast<double>(screenY);
            double dist2 = dx * dx + dy * dy;
            if (dist2 < minDist2) {
                minDist2 = dist2;
                bestMatch = { id, AnglePointRole::Middle };
            }
        }

        // 检查终点
        if (m.endPointActor && m.endPointActor->GetVisibility()) {
            coord->SetValue(m.endPointWorld.data());
            int* dispPos = coord->GetComputedDisplayValue(m_overlayRenderer);
            double dx = dispPos[0] - static_cast<double>(screenX);
            double dy = dispPos[1] - static_cast<double>(screenY);
            double dist2 = dx * dx + dy * dy;
            if (dist2 < minDist2) {
                minDist2 = dist2;
                bestMatch = { id, AnglePointRole::End };
            }
        }
    }

    if (minDist2 <= TOLERANCE_PX * TOLERANCE_PX) {
        return bestMatch;
    }
    return {}; // 未命中
}

void SimpleAngleMeasureManager::UpdateAngleMeasurementPoint(
    int measurementId, AnglePointRole role, const std::array<double, 3>& newWorldPos)
{
    auto it = m_measurements.find(measurementId);
    if (it == m_measurements.end() || !it->second.isComplete) return;

    Measurement& m = it->second;

    // 更新对应点的世界坐标
    switch (role) {
    case AnglePointRole::Start:
        m.startPointWorld = newWorldPos;
        break;
    case AnglePointRole::Middle:
        m.middlePointWorld = newWorldPos;
        break;
    case AnglePointRole::End:
        m.endPointWorld = newWorldPos;
        break;
    default:
        return;
    }

    // === 移除旧的可视化元素 ===
    m_overlayRenderer->RemoveActor(m.startPointActor);
    m_overlayRenderer->RemoveActor(m.middlePointActor);
    m_overlayRenderer->RemoveActor(m.endPointActor);
    m_overlayRenderer->RemoveActor(m.startCrosshairActor);
    m_overlayRenderer->RemoveActor(m.middleCrosshairActor);
    m_overlayRenderer->RemoveActor(m.endCrosshairActor);
    m_overlayRenderer->RemoveActor(m.startToMiddleLine1Actor);
    m_overlayRenderer->RemoveActor(m.middleToEndLineActor);
    m_overlayRenderer->RemoveActor(m.angleArcActor);
    m_overlayRenderer->RemoveViewProp(m.angleLabel);

    // === 重绘新的 ===
    DrawFinalAngleMeasurement(measurementId); // 新增辅助函数
}

void SimpleAngleMeasureManager::DrawFinalAngleMeasurement(int measurementId)
{
    auto it = m_measurements.find(measurementId);
    if (it == m_measurements.end()) return;
    Measurement& m = it->second;

    // 创建点和十字
    m.startPointActor = createSphereActor(m.startPointWorld);
    m.middlePointActor = createSphereActor(m.middlePointWorld);
    m.endPointActor = createSphereActor(m.endPointWorld);
    m.startCrosshairActor = createCrosshairActor(m.startPointWorld, 5.0);
    m.middleCrosshairActor = createCrosshairActor(m.middlePointWorld, 5.0);
    m.endCrosshairActor = createCrosshairActor(m.endPointWorld, 5.0);

    // 创建线段
    m.startToMiddleLine1Actor = createLineActor(m.startPointWorld, m.middlePointWorld);
    m.middleToEndLineActor = createLineActor(m.middlePointWorld, m.endPointWorld);

    // 计算角度和标签
    double angleDeg = ComputeAngle(m.startPointWorld, m.middlePointWorld, m.endPointWorld);
    std::array<double, 3> labelPos = ComputeAngleLabelPosition(
        m.startPointWorld, m.middlePointWorld, m.endPointWorld, 0.3);

    auto cam = m_overlayRenderer->GetActiveCamera();
    if (!cam) return;

    m.angleArcActor = CreateAngleArc(m.startPointWorld, m.middlePointWorld, m.endPointWorld);
    m.angleLabel = CreateAngleLabel(angleDeg, labelPos, cam);

    // 添加到渲染器
    m_overlayRenderer->AddActor(m.startPointActor);
    m_overlayRenderer->AddActor(m.middlePointActor);
    m_overlayRenderer->AddActor(m.endPointActor);
    m_overlayRenderer->AddActor(m.startCrosshairActor);
    m_overlayRenderer->AddActor(m.middleCrosshairActor);
    m_overlayRenderer->AddActor(m.endCrosshairActor);
    m_overlayRenderer->AddActor(m.startToMiddleLine1Actor);
    m_overlayRenderer->AddActor(m.middleToEndLineActor);
    if (m.angleArcActor) {
        m_overlayRenderer->AddActor(m.angleArcActor);
    }
    if (m.angleLabel) {
        m_overlayRenderer->AddViewProp(m.angleLabel);
    }
}