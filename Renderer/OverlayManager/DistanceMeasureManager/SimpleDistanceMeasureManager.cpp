#include "SimpleDistanceMeasureManager.h"
#include <vtkRenderer.h>
#include <vtkImageViewer2.h>
#include <QDebug>
#include <vtkCamera.h>
#include <sstream>
#include <iomanip>
#include <vtkSphereSource.h>
#include <vtkRenderWindow.h>
#include <vtkMath.h>
#include <vtkAppendPolyData.h>

SimpleDistanceMeasureManager::SimpleDistanceMeasureManager() = default;

SimpleDistanceMeasureManager::~SimpleDistanceMeasureManager() {
    Shutdown();
}

void SimpleDistanceMeasureManager::Initialize(vtkRenderer* overlayRenderer) {
    if (m_initialized) return;
    if (!overlayRenderer) return;
    m_overlayRenderer = overlayRenderer;
    m_initialized = true;
    qDebug() << "[SimpleDistanceMeasureManager] Initialized";
}

void SimpleDistanceMeasureManager::StartMeasure(const std::array<double, 3>& startPoint) {
    qDebug() << "[SimpleDistanceMeasureManager] StartMeasure - Point:" << startPoint[0] << startPoint[1] << startPoint[2];
}

void SimpleDistanceMeasureManager::UpdateMeasure(const std::array<double, 3>& endPoint) {
    qDebug() << "[SimpleDistanceMeasureManager] UpdateMeasure - Point:" << endPoint[0] << endPoint[1] << endPoint[2];
}

void SimpleDistanceMeasureManager::EndMeasure() {
    qDebug() << "[SimpleDistanceMeasureManager] EndMeasure";
}

void SimpleDistanceMeasureManager::SetVisible(bool visible) {
    m_visible = visible;
    qDebug() << "[SimpleDistanceMeasureManager] SetVisible:" << visible;
}

void SimpleDistanceMeasureManager::Shutdown() {
    if (!m_initialized) return;
    qDebug() << "[SimpleDistanceMeasureManager] Shutdown";
    m_overlayRenderer = nullptr;
    m_initialized = false;
}

void SimpleDistanceMeasureManager::DrawStartPoint(std::array<double, 3> worldPoint) {
    if (!m_overlayRenderer) return;
    MeasurementID id = generateNextId();
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

void SimpleDistanceMeasureManager::DrawFinalMeasurementLine(std::array<double, 3> startPos, std::array<double, 3> endPos) {
    if (!m_overlayRenderer) return;
    ClearPreview();

    auto it = m_measurements.find(m_nextId);
    if (it == m_measurements.end()) return;
    Measurement& m = it->second;

    m.endPointWorld = endPos;
    m.endPointActor = createSphereActor(endPos);
    m.endCrosshairActor = createCrosshairActor(endPos, 5.0);
    m.isComplete = true;

    // 创建正式测量的固定线（不可更新）
    m.lineActor = createLineActor(startPos, endPos);
    m.tickActor = createTickActor(startPos, endPos);
    auto cam = m_overlayRenderer->GetActiveCamera();
    m.distanceLabel = createDistanceLabel(startPos, endPos, cam);

    m_overlayRenderer->AddActor(m.endPointActor);
    m_overlayRenderer->AddActor(m.endCrosshairActor);
    m_overlayRenderer->AddViewProp(m.lineActor);
    m_overlayRenderer->AddViewProp(m.tickActor);
    m_overlayRenderer->AddViewProp(m.distanceLabel);
}

void SimpleDistanceMeasureManager::PreviewMeasurementLine(
    std::array<double, 3> startPos,
    std::array<double, 3> currentPos)
{
    if (!m_overlayRenderer || !m_initialized) return;
    auto cam = m_overlayRenderer->GetActiveCamera();
    if (!cam) return;

    // === 懒初始化：只创建一次 ===
    if (!m_previewLineActor) {
        m_previewLineSource = vtkSmartPointer<vtkLineSource>::New();
        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(m_previewLineSource->GetOutputPort());
        m_previewLineActor = vtkSmartPointer<vtkActor>::New();
        m_previewLineActor->SetMapper(mapper);
        m_previewLineActor->GetProperty()->SetColor(0.0, 1.0, 0.0);
        m_overlayRenderer->AddViewProp(m_previewLineActor);
    }

    if (!m_previewTickActor) {
        m_previewTickSource = vtkSmartPointer<vtkLineSource>::New();
        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(m_previewTickSource->GetOutputPort());
        m_previewTickActor = vtkSmartPointer<vtkActor>::New();
        m_previewTickActor->SetMapper(mapper);
        m_previewTickActor->GetProperty()->SetColor(1.0, 1.0, 1.0);
        m_previewTickActor->GetProperty()->SetLineWidth(4.0);
        m_overlayRenderer->AddViewProp(m_previewTickActor);
    }

    if (!m_previewLabelActor) {
        m_previewTextSource = vtkSmartPointer<vtkVectorText>::New();
        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(m_previewTextSource->GetOutputPort());
        m_previewLabelActor = vtkSmartPointer<vtkFollower>::New();
        m_previewLabelActor->SetMapper(mapper);
        m_previewLabelActor->GetProperty()->SetColor(1.0, 1.0, 1.0);
        m_previewLabelActor->SetScale(8.0, 8.0, 8.0);
        m_previewLabelActor->SetCamera(cam);
        m_overlayRenderer->AddViewProp(m_previewLabelActor);
    }

    // === 更新数据（零重建）===
    m_previewLineSource->SetPoint1(startPos.data());
    m_previewLineSource->SetPoint2(currentPos.data());
    m_previewLineSource->Modified();

    updatePreviewTick(startPos, currentPos, 3.0);
    updatePreviewLabel(startPos, currentPos, cam);
}

void SimpleDistanceMeasureManager::ClearPreview()
{
    if (!m_overlayRenderer) return;

    if (m_previewLineActor)      m_overlayRenderer->RemoveViewProp(m_previewLineActor);
    if (m_previewTickActor)      m_overlayRenderer->RemoveViewProp(m_previewTickActor);
    if (m_previewLabelActor)     m_overlayRenderer->RemoveViewProp(m_previewLabelActor);

    // 解除引用（触发释放）
    m_previewLineSource = nullptr;
    m_previewLineActor = nullptr;
    m_previewTickSource = nullptr;
    m_previewTickActor = nullptr;
    m_previewTextSource = nullptr;
    m_previewLabelActor = nullptr;
}

void SimpleDistanceMeasureManager::ClearAllMeasurement() {
    if (!m_overlayRenderer) return;

    for (auto& pair : m_measurements) {
        Measurement& m = pair.second;
        m_overlayRenderer->RemoveViewProp(m.startPointActor);
        m_overlayRenderer->RemoveViewProp(m.endPointActor);
        m_overlayRenderer->RemoveViewProp(m.startCrosshairActor);
        m_overlayRenderer->RemoveViewProp(m.endCrosshairActor);
        m_overlayRenderer->RemoveViewProp(m.lineActor);
        m_overlayRenderer->RemoveViewProp(m.tickActor);
        m_overlayRenderer->RemoveViewProp(m.distanceLabel);
    }
    m_measurements.clear();
    m_nextId = 0;
}

void SimpleDistanceMeasureManager::ClearCurrentMeasurement() {

    ClearPreview();
    auto it = m_measurements.find(m_nextId);
    if (it == m_measurements.end()) return;
    Measurement& m = it->second;
    m_overlayRenderer->RemoveViewProp(m.startPointActor);
    m_overlayRenderer->RemoveViewProp(m.startCrosshairActor);
    m_overlayRenderer->RemoveViewProp(m.lineActor);
    m_overlayRenderer->RemoveViewProp(m.tickActor);
    m_overlayRenderer->RemoveViewProp(m.distanceLabel);
}

// ================== 工厂函数（保持不变）==================
vtkSmartPointer<vtkActor> SimpleDistanceMeasureManager::createSphereActor(const std::array<double, 3>& point) {
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

vtkSmartPointer<vtkActor> SimpleDistanceMeasureManager::createLineActor(const std::array<double, 3>& startPoint, const std::array<double, 3>& endPoint) {
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

vtkSmartPointer<vtkActor> SimpleDistanceMeasureManager::createCrosshairActor(const std::array<double, 3>& center, double length) {
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

vtkSmartPointer<vtkActor> SimpleDistanceMeasureManager::createTickActor(
    const std::array<double, 3>& p1,
    const std::array<double, 3>& p2,
    double tickLength)
{
    double dx = p2[0] - p1[0];
    double dy = p2[1] - p1[1];
    double dz = p2[2] - p1[2];
    double len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len < 1e-6) return nullptr;

    double mid[3] = { (p1[0] + p2[0]) / 2.0, (p1[1] + p2[1]) / 2.0, (p1[2] + p2[2]) / 2.0 };
    double dir[3] = { dx / len, dy / len, dz / len };

    double ref[3];
    if (std::abs(dir[2]) < 0.9) {
        ref[0] = 0.0; ref[1] = 0.0; ref[2] = 1.0;
    }
    else {
        ref[0] = 1.0; ref[1] = 0.0; ref[2] = 0.0;
    }

    double perp[3];
    vtkMath::Cross(dir, ref, perp);
    double perpLen = vtkMath::Norm(perp);
    if (perpLen < 1e-6) {
        ref[0] = 0.0; ref[1] = 1.0; ref[2] = 0.0;
        vtkMath::Cross(dir, ref, perp);
        perpLen = vtkMath::Norm(perp);
        if (perpLen < 1e-6) return nullptr;
    }
    vtkMath::Normalize(perp);

    double start[3] = {
        mid[0] - (tickLength / 2.0) * perp[0],
        mid[1] - (tickLength / 2.0) * perp[1],
        mid[2] - (tickLength / 2.0) * perp[2]
    };
    double end[3] = {
        mid[0] + (tickLength / 2.0) * perp[0],
        mid[1] + (tickLength / 2.0) * perp[1],
        mid[2] + (tickLength / 2.0) * perp[2]
    };

    auto source = vtkSmartPointer<vtkLineSource>::New();
    source->SetPoint1(start);
    source->SetPoint2(end);
    source->Update();

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(source->GetOutputPort());
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1.0, 1.0, 1.0);
    actor->GetProperty()->SetLineWidth(4.0);
    return actor;
}

vtkSmartPointer<vtkFollower> SimpleDistanceMeasureManager::createDistanceLabel(
    const std::array<double, 3>& p1,
    const std::array<double, 3>& p2,
    vtkCamera* camera,
    double scale,
    double offset)
{
    double dx = p2[0] - p1[0];
    double dy = p2[1] - p1[1];
    double dz = p2[2] - p1[2];
    double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    double mid[3] = { (p1[0] + p2[0]) / 2.0, (p1[1] + p2[1]) / 2.0, (p1[2] + p2[2]) / 2.0 };

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << distance << " mm";
    auto textSource = vtkSmartPointer<vtkVectorText>::New();
    textSource->SetText(oss.str().c_str());

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(textSource->GetOutputPort());

    auto follower = vtkSmartPointer<vtkFollower>::New();
    follower->SetMapper(mapper);
    follower->GetProperty()->SetColor(1.0, 1.0, 1.0);
    follower->SetScale(scale, scale, scale);

    if (distance > 1e-6) {
        double dir[3] = { dx / distance, dy / distance, dz / distance };
        double perp[3];
        vtkMath::Perpendiculars(dir, perp, nullptr, 0);
        double pos[3] = {
            mid[0] + (1.5 + offset) * perp[0],
            mid[1] + (1.5 + offset) * perp[1],
            mid[2] + (1.5 + offset) * perp[2]
        };
        follower->SetPosition(pos);
    }
    else {
        follower->SetPosition(mid);
    }
    if (camera) {
        follower->SetCamera(camera);
    }
    return follower;
}

// ================== 新增：预览更新辅助函数 ==================
void SimpleDistanceMeasureManager::updatePreviewTick(
    const std::array<double, 3>& p1,
    const std::array<double, 3>& p2,
    double tickLength)
{
    double dx = p2[0] - p1[0];
    double dy = p2[1] - p1[1];
    double dz = p2[2] - p1[2];
    double len = std::sqrt(dx * dx + dy * dy + dz * dz);

    double mid[3] = { (p1[0] + p2[0]) / 2.0, (p1[1] + p2[1]) / 2.0, (p1[2] + p2[2]) / 2.0 };

    if (len < 1e-6) {
        m_previewTickSource->SetPoint1(mid);
        m_previewTickSource->SetPoint2(mid);
    }
    else {
        double dir[3] = { dx / len, dy / len, dz / len };
        double ref[3];
        if (std::abs(dir[2]) < 0.9) {
            ref[0] = 0.0; ref[1] = 0.0; ref[2] = 1.0;
        }
        else {
            ref[0] = 1.0; ref[1] = 0.0; ref[2] = 0.0;
        }

        double perp[3];
        vtkMath::Cross(dir, ref, perp);
        double perpLen = vtkMath::Norm(perp);
        if (perpLen < 1e-6) {
            ref[0] = 0.0; ref[1] = 1.0; ref[2] = 0.0;
            vtkMath::Cross(dir, ref, perp);
            perpLen = vtkMath::Norm(perp);
        }
        if (perpLen > 1e-6) {
            vtkMath::Normalize(perp);
        }

        double start[3] = {
            mid[0] - (tickLength / 2.0) * perp[0],
            mid[1] - (tickLength / 2.0) * perp[1],
            mid[2] - (tickLength / 2.0) * perp[2]
        };
        double end[3] = {
            mid[0] + (tickLength / 2.0) * perp[0],
            mid[1] + (tickLength / 2.0) * perp[1],
            mid[2] + (tickLength / 2.0) * perp[2]
        };
        m_previewTickSource->SetPoint1(start);
        m_previewTickSource->SetPoint2(end);
    }
    m_previewTickSource->Modified();
}

void SimpleDistanceMeasureManager::updatePreviewLabel(
    const std::array<double, 3>& p1,
    const std::array<double, 3>& p2,
    vtkCamera* camera)
{
    double dx = p2[0] - p1[0];
    double dy = p2[1] - p1[1];
    double dz = p2[2] - p1[2];
    double distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << distance << " mm";
    m_previewTextSource->SetText(oss.str().c_str());
    m_previewTextSource->Modified();

    double mid[3] = { (p1[0] + p2[0]) / 2.0, (p1[1] + p2[1]) / 2.0, (p1[2] + p2[2]) / 2.0 };

    if (distance > 1e-6) {
        double dir[3] = { dx / distance, dy / distance, dz / distance };
        double perp[3];
        vtkMath::Perpendiculars(dir, perp, nullptr, 0);
        double offset = 2.5;
        double pos[3] = {
            mid[0] + offset * perp[0],
            mid[1] + offset * perp[1],
            mid[2] + offset * perp[2]
        };
        m_previewLabelActor->SetPosition(pos);
    }
    else {
        m_previewLabelActor->SetPosition(mid);
    }
}