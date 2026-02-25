#pragma once
#include "IDistanceMeasureManager.h"
#include <vtkSmartPointer.h>
#include <array>
#include <vtkActor.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper.h>
#include "../IOverlayFeature.h"

// 新增包含
#include <vtkVectorText.h>
#include <vtkFollower.h>
#include <vtkProperty.h>
class vtkRenderer;
class vtkImageViewer2;

/// @brief 距离测量工具管理器实现
class SimpleDistanceMeasureManager : public IDistanceMeasureManager{
public:
    SimpleDistanceMeasureManager();
    ~SimpleDistanceMeasureManager() override;

    void Initialize(vtkRenderer* overlayRenderer) override;
    void StartMeasure(const std::array<double, 3>& startPoint) override;
    void UpdateMeasure(const std::array<double, 3>& endPoint) override;
    void EndMeasure() override;
    void SetVisible(bool visible) override;
    void Shutdown() override;

    void DrawStartPoint(std::array<double, 3> worldPoint) override; // 新增方法
    void DrawFinalMeasurementLine(std::array<double, 3> startPos, std::array<double, 3> endPos)override;
    void PreviewMeasurementLine(std::array<double, 3> startPos, std::array<double, 3> currentPos)override;
    void ClearMeasurement()override;
private:
    vtkSmartPointer<vtkRenderer> m_overlayRenderer;
    vtkImageViewer2* m_viewer = nullptr;
    bool m_initialized = false;
    bool m_visible = true;

    std::vector<vtkSmartPointer<vtkActor>> m_distanceActors; // 用于保存所有actor的容器

    // 👇 新增：用于预览线的专用成员（复用）
    vtkSmartPointer<vtkLineSource> m_previewLineSource;
    vtkSmartPointer<vtkPolyDataMapper> m_previewMapper;
    vtkSmartPointer<vtkActor> m_previewLineActor;

    // 👇 新增：距离文本
    vtkSmartPointer<vtkVectorText> m_distanceTextSource;
    vtkSmartPointer<vtkPolyDataMapper> m_textMapper;
    vtkSmartPointer<vtkFollower> m_distanceTextActor; // 使用 Follower！

    vtkSmartPointer<vtkActor> m_finalLineActor;
    vtkSmartPointer<vtkActor> m_midTickActor;
    vtkSmartPointer<vtkFollower> m_distanceLabelActor;

    vtkSmartPointer<vtkActor> m_previewMidTickActor;      // 👈 新增：预览刻度线
    vtkSmartPointer<vtkFollower> m_previewDistanceLabel;  // 👈 新增：预览文本
};