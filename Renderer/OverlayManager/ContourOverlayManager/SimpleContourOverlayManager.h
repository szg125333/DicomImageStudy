#pragma once

#include "Renderer/OverlayManager/IOverlayFeature.h"
#include "Utils/RtStructReader.h"

#include <vector>
#include <vtkSmartPointer.h>

class vtkRenderer;
class vtkImageViewer2;
class vtkActor;

/**
 * @brief RT Structure 轮廓叠加显示 Feature
 *
 * 功能：
 *   - 接收 RtStructReader 读取的 ROI 数据
 *   - 预先为每条轮廓创建 vtkActor（初始隐藏）
 *   - 切片切换时：隐藏所有轮廓，只显示 Z 坐标与当前切片匹配的轮廓
 *   - 颜色来自 RT-S 文件的 ROI Display Color（3006|002A）
 *
 * 注意：
 *   RT-S 坐标系为 LPS（mm）。如果 CT 图像经过 vtkImageReslice 重采样，
 *   坐标已对齐，直接使用世界坐标即可。
 *   若未重采样，需要在 GetSliceWorldZ 中做坐标变换（本实现假设已对齐）。
 */
class SimpleContourOverlayManager : public IOverlayFeature {
public:
    SimpleContourOverlayManager();
    ~SimpleContourOverlayManager() override;

    void Initialize(vtkRenderer* overlayRenderer)   override;
    void SetVisible(bool visible)                   override;
    void SetColor(double r, double g, double b)     override;
    void Shutdown()                                 override;
    void OnSliceChanged(vtkImageViewer2* viewer,
        int slice,
        ViewType viewType)           override;

    /// @brief 设置 ROI 数据，预先创建所有 Actor
    void SetRois(const std::vector<RtRoi>& rois);

    /// @brief 清除所有数据和 Actor
    void ClearAll();

    /// @brief 设置切片 Z 匹配公差（mm，默认 0.5）
    void SetSliceMatchTolerance(double mm) { m_tolerance = mm; }

    bool HasData() const { return !m_rois.empty(); }

private:
    struct ContourRecord {
        vtkSmartPointer<vtkActor> actor;
        double   sliceZ = 0.0;
        ViewType viewType = ViewType::None;
    };

    void BuildActors();
    void RemoveAllActors();
    void UpdateVisibleContours(double sliceZ, ViewType viewType);

    vtkSmartPointer<vtkActor> CreateContourActor(
        const std::vector<std::array<double, 3>>& pts,
        const std::array<double, 3>& color,
        double fixedZ
        /*int    fixedAxisIndex*/);   // 0=X(Sagittal) 1=Y(Coronal) 2=Z(Axial)

    double GetSliceWorldZ(vtkImageViewer2* viewer,
        int slice, ViewType viewType) const;

    vtkRenderer* m_overlayRenderer = nullptr;
    bool         m_initialized = false;
    bool         m_visible = true;

    std::vector<RtRoi>          m_rois;
    std::vector<ContourRecord>  m_records;

    double m_tolerance = 0.5;
};
