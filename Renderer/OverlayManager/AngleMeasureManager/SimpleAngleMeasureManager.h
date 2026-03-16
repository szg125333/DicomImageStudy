#pragma once

#include "IAngleMeasureManager.h"
#include "Common/Measurement/MeasurementTypes.h"

// 前置声明（避免包含重量级头文件）
class vtkRenderer;
class vtkImageViewer2;
class vtkActor;
class vtkFollower;
class vtkLineSource;
class vtkPoints;
class vtkPolyLine;
class vtkCellArray;
class vtkPolyData;
class vtkVectorText;
class vtkCamera;

#include <vtkSmartPointer.h>
#include <array>
#include <unordered_map>

/// @brief 角度测量工具管理器实现
class SimpleAngleMeasureManager : public IAngleMeasureManager {
public:
    // === 构造与生命周期 ===
    SimpleAngleMeasureManager();
    ~SimpleAngleMeasureManager() override;

    void Initialize(vtkRenderer* overlayRenderer) override;
    void Shutdown() override;
    void SetVisible(bool visible) override;

    // === 测量流程控制 ===
    void StartMeasure(const std::array<double, 3>& point1) override;
    void UpdateMeasure(const std::array<double, 3>& point2) override;
    void EndMeasure(const std::array<double, 3>& point3) override;

    // === 样式 ===
    void SetColor(double r, double g, double b) override;

    // === 清理 ===
    void ClearAllMeasurement() override;
    void ClearCurrentMeasurement() override;
    void ClearPreview();

    // === 交互编辑支持 ===
    EditableAnglePoint GetEditableAnglePoint(int screenX, int screenY) const;
    void UpdateAngleMeasurementPoint(int measurementId, AnglePointRole role, const std::array<double, 3>& newWorldPos);

    // === 公共工具函数（供外部或测试使用）===
    static double ComputeAngle(
        const std::array<double, 3>& startPoint,
        const std::array<double, 3>& vertexPoint,
        const std::array<double, 3>& endPoint);

    static std::array<double, 3> ComputeAngleLabelPosition(
        const std::array<double, 3>& start,
        const std::array<double, 3>& vertex,
        const std::array<double, 3>& end,
        double offsetFactor = 0.3);

    vtkSmartPointer<vtkActor> CreateAngleArc(
        const std::array<double, 3>& startPoint,
        const std::array<double, 3>& vertexPoint,
        const std::array<double, 3>& endPoint);

    vtkSmartPointer<vtkFollower> CreateAngleLabel(
        double angleDeg,
        const std::array<double, 3>& labelPosition,
        vtkCamera* camera);

    void DrawStartPoint(std::array<double, 3> worldPoint);
    void DrawMiddlePointAndStartToMiddleLine(std::array<double, 3> worldPoint);
    void DrawEndPointAndMiddleToEndLine(std::array<double, 3> worldPoint);

    void PreviewStartToMiddleMeasurementLine(std::array<double, 3> startPos, std::array<double, 3> currentPos);
    void PreviewMiddleToEndMeasurementLine(std::array<double, 3> startPos, std::array<double, 3> currentPos);

private:
    void UpdatePreviewAngleArc(
        const std::array<double, 3>& start,
        const std::array<double, 3>& vertex,
        const std::array<double, 3>& end);

    void UpdatePreviewAngleLabel(
        double angleDeg,
        const std::array<double, 3>& start,
        const std::array<double, 3>& vertex,
        const std::array<double, 3>& end,
        vtkCamera* camera);

    void DrawFinalAngleMeasurement(int measurementId);

    void GenerateArcPoints(
        const std::array<double, 3>& startPoint,
        const std::array<double, 3>& vertexPoint,
        const std::array<double, 3>& endPoint,
        vtkPoints* points,
        vtkPolyLine* polyLine);

    // === 工厂函数：创建基础图元 ===
    vtkSmartPointer<vtkActor> createSphereActor(const std::array<double, 3>& point);
    vtkSmartPointer<vtkActor> createLineActor(const std::array<double, 3>& p1, const std::array<double, 3>& p2);
    vtkSmartPointer<vtkActor> createCrosshairActor(const std::array<double, 3>& center, double length = 10.0);
    vtkSmartPointer<vtkActor> createTickActor(const std::array<double, 3>& p1, const std::array<double, 3>& p2, double tickLength = 3.0);
    vtkSmartPointer<vtkFollower> createDistanceLabel(const std::array<double, 3>& p1, const std::array<double, 3>& p2, vtkCamera* camera, double scale = 8.0, double offset = 2.0);

    // === 数据结构 ===
    struct Measurement {
        int id = -1;

        // 世界坐标（用于计算）
        std::array<double, 3> startPointWorld;
        std::array<double, 3> middlePointWorld;
        std::array<double, 3> endPointWorld;

        // 可视化组件
        vtkSmartPointer<vtkActor> startPointActor;
        vtkSmartPointer<vtkActor> startCrosshairActor;
        vtkSmartPointer<vtkActor> middlePointActor;
        vtkSmartPointer<vtkActor> middleCrosshairActor;
        vtkSmartPointer<vtkActor> endPointActor;
        vtkSmartPointer<vtkActor> endCrosshairActor;

        vtkSmartPointer<vtkActor> startToMiddleLine1Actor;
        vtkSmartPointer<vtkActor> middleToEndLineActor;

        vtkSmartPointer<vtkActor> angleArcActor;
        vtkSmartPointer<vtkFollower> angleLabel;

        bool isComplete = false;
    };

    // === 成员变量 ===
    std::unordered_map<int, Measurement> m_measurements;
    int m_currentId = 0;
    int generateNextId() { return ++m_currentId; }

    // 预览专用（不存入 m_measurements）
    vtkSmartPointer<vtkLineSource> m_previewStartToMiddleLineSource;
    vtkSmartPointer<vtkActor>      m_previewStartToMiddleLineActor;
    vtkSmartPointer<vtkLineSource> m_previewMiddleToEndLineSource;
    vtkSmartPointer<vtkActor>      m_previewMiddleToEndLineActor;

    vtkSmartPointer<vtkActor> m_previewAngleArcActor;
    vtkSmartPointer<vtkPoints> m_previewArcPoints;
    vtkSmartPointer<vtkPolyLine> m_previewArcPolyLine;
    vtkSmartPointer<vtkCellArray> m_previewArcCells;
    vtkSmartPointer<vtkPolyData> m_previewArcPolyData;
    vtkSmartPointer<vtkFollower> m_previewAngleLabel;
    vtkSmartPointer<vtkVectorText> m_previewLabelText;

    vtkSmartPointer<vtkRenderer> m_overlayRenderer;
    vtkImageViewer2* m_viewer = nullptr;
    bool m_initialized = false;
    bool m_visible = true;
};