#include "SimpleDistanceMeasureManager.h"
#include <vtkRenderer.h>
#include <vtkImageViewer2.h>
#include <QDebug>

#include <vtkCamera.h>
#include <sstream>
#include <iomanip>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkProperty.h>
#include <vtkMath.h>
#include <vtkVectorText.h>
#include <vtkFollower.h>
SimpleDistanceMeasureManager::SimpleDistanceMeasureManager() = default;

SimpleDistanceMeasureManager::~SimpleDistanceMeasureManager() {
    Shutdown();
}

void SimpleDistanceMeasureManager::Initialize(vtkRenderer* overlayRenderer) {
    if (m_initialized) return;
    if (!overlayRenderer) return;

    m_overlayRenderer = overlayRenderer;
    //m_viewer = viewer;
    m_initialized = true;

    qDebug() << "[SimpleDistanceMeasureManager] Initialized";
}

void SimpleDistanceMeasureManager::StartMeasure(const std::array<double, 3>& startPoint) {
    qDebug() << "[SimpleDistanceMeasureManager] StartMeasure - Point:"
        << startPoint[0] << startPoint[1] << startPoint[2];
}

void SimpleDistanceMeasureManager::UpdateMeasure(const std::array<double, 3>& endPoint) {
    qDebug() << "[SimpleDistanceMeasureManager] UpdateMeasure - Point:"
        << endPoint[0] << endPoint[1] << endPoint[2];
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
    //m_viewer = nullptr;
    m_initialized = false;
}

void SimpleDistanceMeasureManager::DrawStartPoint(std::array<double, 3> worldPoint)
{
    if (!m_overlayRenderer) return;

    double picked[3] = { worldPoint[0],worldPoint[1],worldPoint[2] };

    // 2. 创建球体表示起点
    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetCenter(picked);
    sphereSource->SetRadius(1.0); // 单位：mm（与你的数据空间一致）
    sphereSource->SetPhiResolution(16);
    sphereSource->SetThetaResolution(16);
    sphereSource->Update();

    // 3. 创建 Mapper 和 Actor
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(sphereSource->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1.0, 0.0, 0.0); // 红色
    actor->GetProperty()->SetLighting(false);       // 可选：关闭光照，颜色更纯

    // 4. 添加到渲染器并重绘
    m_overlayRenderer->AddActor(actor);
    m_distanceActors.push_back(actor); // 也保存到距离测量的 actor 列表中
}

// 在 VtkViewRenderer.cpp 中实现这些方法
void SimpleDistanceMeasureManager::DrawFinalMeasurementLine(std::array<double, 3> startPos, std::array<double, 3> endPos) {
    // 实现具体的绘制逻辑
    //PreviewMeasurementLine(startPos, endPos);
    DrawStartPoint(endPos);
}

void SimpleDistanceMeasureManager::PreviewMeasurementLine(
    std::array<double, 3> startPos,
    std::array<double, 3> currentPos
) {
    if (!m_overlayRenderer || !m_initialized) return;

    // === 1. 计算距离和中点 ===
    double dx = currentPos[0] - startPos[0];
    double dy = currentPos[1] - startPos[1];
    double dz = currentPos[2] - startPos[2];
    double distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    double midPoint[3] = {
        (startPos[0] + currentPos[0]) / 2.0,
        (startPos[1] + currentPos[1]) / 2.0,
        (startPos[2] + currentPos[2]) / 2.0
    };

    // === 2. 更新/创建预览主线 ===
    if (!m_previewLineActor) {
        m_previewLineSource = vtkSmartPointer<vtkLineSource>::New();
        m_previewMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        m_previewLineActor = vtkSmartPointer<vtkActor>::New();

        m_previewMapper->SetInputConnection(m_previewLineSource->GetOutputPort());
        m_previewLineActor->SetMapper(m_previewMapper);
        m_previewLineActor->GetProperty()->SetColor(0.0, 1.0, 0.0); // 绿色
        m_previewLineActor->GetProperty()->SetLineWidth(2.0);
        m_overlayRenderer->AddActor(m_previewLineActor);
        m_distanceActors.push_back(m_previewLineActor);
    }
    m_previewLineSource->SetPoint1(startPos.data());
    m_previewLineSource->SetPoint2(currentPos.data());
    m_previewLineSource->Update();

    // === 3. 更新/创建中点刻度线 ===
    if (!m_previewMidTickActor) {
        m_previewMidTickActor = vtkSmartPointer<vtkActor>::New();
        auto tickMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        m_previewMidTickActor->SetMapper(tickMapper);
        m_previewMidTickActor->GetProperty()->SetColor(1.0, 1.0, 1.0); // 白色
        m_previewMidTickActor->GetProperty()->SetLineWidth(4.0);
        m_overlayRenderer->AddActor(m_previewMidTickActor);
        m_distanceActors.push_back(m_previewMidTickActor);
    }

    // 计算垂直方向
    double dir[3] = { dx, dy, dz };
    vtkMath::Normalize(dir);
    double perp[3];
    vtkMath::Perpendiculars(dir, perp, nullptr, 0);

    const double tickLength = 3.0; // mm
    double tickStart[3] = {
        midPoint[0] - tickLength / 2 * perp[0],
        midPoint[1] - tickLength / 2 * perp[1],
        midPoint[2] - tickLength / 2 * perp[2]
    };
    double tickEnd[3] = {
        midPoint[0] + tickLength / 2 * perp[0],
        midPoint[1] + tickLength / 2 * perp[1],
        midPoint[2] + tickLength / 2 * perp[2]
    };

    // 动态更新刻度线（复用 mapper 的 input）
    auto tickSource = vtkSmartPointer<vtkLineSource>::New();
    tickSource->SetPoint1(tickStart);
    tickSource->SetPoint2(tickEnd);
    tickSource->Update();
    m_previewMidTickActor->GetMapper()->SetInputConnection(tickSource->GetOutputPort());

    // === 4. 更新/创建距离文本 ===
    if (!m_previewDistanceLabel) {
        m_previewDistanceLabel = vtkSmartPointer<vtkFollower>::New();
        auto textMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        m_previewDistanceLabel->SetMapper(textMapper);
        m_previewDistanceLabel->GetProperty()->SetColor(1.0, 1.0, 1.0); // 白色
        m_previewDistanceLabel->SetScale(8.0, 8.0, 8.0);
        m_overlayRenderer->AddActor(m_previewDistanceLabel);
        m_distanceActors.push_back(m_previewDistanceLabel);
    }

    // 格式化文本
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << distance << " mm";
    auto textSource = vtkSmartPointer<vtkVectorText>::New();
    textSource->SetText(oss.str().c_str());
    textSource->Update();
    m_previewDistanceLabel->GetMapper()->SetInputConnection(textSource->GetOutputPort());

    // 设置文本位置（从中点沿垂直方向偏移）
    double labelPos[3] = {
        midPoint[0] + (tickLength / 2 + 2.0) * perp[0],
        midPoint[1] + (tickLength / 2 + 2.0) * perp[1],
        midPoint[2] + (tickLength / 2 + 2.0) * perp[2]
    };
    m_previewDistanceLabel->SetPosition(labelPos);
    if (auto cam = m_overlayRenderer->GetActiveCamera()) {
        m_previewDistanceLabel->SetCamera(cam);
    }
}
void SimpleDistanceMeasureManager::ClearMeasurement() {
    // 清除测量相关的绘制
}