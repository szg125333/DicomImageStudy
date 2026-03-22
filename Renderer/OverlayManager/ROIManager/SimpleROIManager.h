#pragma once

#include "Renderer/OverlayManager/IOverlayFeature.h"
#include "RoiStats.h"
#include "RoiLabel.h"

#include <array>
#include <unordered_map>
#include <vtkSmartPointer.h>

class vtkRenderer;
class vtkImageViewer2;
class vtkActor;
class vtkCamera;
class vtkLineSource;
class vtkPolyDataMapper;
class vtkPoints;

// ----------------------------------------------------------------
//  拖动命中类型
// ----------------------------------------------------------------

enum class RoiHitType {
    None,
    Body,       ///< 边框或内部（整体平移）
    CornerTL,   ///< 左上角
    CornerTR,   ///< 右上角
    CornerBL,   ///< 左下角
    CornerBR,   ///< 右下角
};

struct RoiHitResult {
    int        roiId = -1;
    RoiHitType hitType = RoiHitType::None;
};

// ----------------------------------------------------------------
//  SimpleROIManager
// ----------------------------------------------------------------

/**
 * @brief ROI 矩形框 Overlay Feature
 *
 * 功能：
 *   - 左键拖拽绘制矩形框，释放后固定并计算统计
 *   - 统计标签以多行形式显示在矩形框正下方（每行独立 vtkFollower）
 *   - 支持整体拖动（命中边框/内部）和角点缩放（命中四角）
 *   - 切片切换后坐标自动投影，统计值重算
 *
 * 统计标签格式（6 行）：
 *   第1行  Mean: 45.2 HU
 *   第2行  SD: 12.1
 *   第3行  Min: -500.0 HU
 *   第4行  Max: 120.0 HU
 *   第5行  Area: 1234.5 mm2
 *   第6行  Pixels: 512
 */
class SimpleROIManager : public IOverlayFeature {
public:
    SimpleROIManager();
    ~SimpleROIManager() override;

    // ----------------------------------------------------------------
    //  IOverlayFeature 接口
    // ----------------------------------------------------------------
    void Initialize(vtkRenderer* overlayRenderer)           override;
    void SetVisible(bool visible)                           override;
    void SetColor(double r, double g, double b)             override;
    void Shutdown()                                         override;
    void OnSliceChanged(vtkImageViewer2* viewer,
        int slice,
        ViewType viewType)                  override;

    // ----------------------------------------------------------------
    //  绘制接口
    // ----------------------------------------------------------------
    void BeginROI(const std::array<double, 3>& cornerWorld,
        ViewType viewType, int slice);
    void UpdatePreview(const std::array<double, 3>& oppositeCorner);
    void CommitROI(const std::array<double, 3>& oppositeCorner,
        vtkImageViewer2* viewer);
    void CancelCurrentROI();

    // ----------------------------------------------------------------
    //  拖动接口
    // ----------------------------------------------------------------
    RoiHitResult HitTest(int screenX, int screenY) const;
    void MoveROI(int roiId,
        const std::array<double, 3>& delta,
        vtkImageViewer2* viewer);
    void ResizeROI(int roiId,
        RoiHitType hitType,
        const std::array<double, 3>& newCornerWorld,
        vtkImageViewer2* viewer);

    // ----------------------------------------------------------------
    //  删除接口
    // ----------------------------------------------------------------
    void DeleteLastROI();
    void ClearAllROI();

    // ----------------------------------------------------------------
    //  查询接口
    // ----------------------------------------------------------------
    RoiStats GetLastStats() const;
    RoiStats GetStats(int roiId) const;
    bool     IsDrawing() const { return m_isDrawing; }

private:
    // ----------------------------------------------------------------
    //  内部数据结构
    // ----------------------------------------------------------------

    struct RoiRecord {
        int      id = -1;
        ViewType viewType = ViewType::None;
        int      slice = 0;

        /// 轴对齐最小角点（世界坐标）
        std::array<double, 3> corner1 = { 0, 0, 0 };
        /// 轴对齐最大角点（世界坐标）
        std::array<double, 3> corner2 = { 0, 0, 0 };

        RoiStats stats;
        bool     isComplete = false;

        // 边框：4 条 LineSource（支持零重建更新）
        struct BorderLine {
            vtkSmartPointer<vtkLineSource>     source;
            vtkSmartPointer<vtkPolyDataMapper> mapper;
            vtkSmartPointer<vtkActor>          actor;
        };
        BorderLine borderLines[4];

        // 半透明填充面
        vtkSmartPointer<vtkActor> fillActor;

        // 四个角点方块（命中测试视觉提示）
        vtkSmartPointer<vtkActor> cornerActors[4];

        // 多行统计标签（框正下方，每行独立 vtkFollower）
        RoiLabel label;
    };

    // ----------------------------------------------------------------
    //  私有方法
    // ----------------------------------------------------------------

    void ComputeRectCorners(const std::array<double, 3>& c1,
        const std::array<double, 3>& c2,
        ViewType viewType,
        std::array<double, 3> out[4]) const;

    /// 计算标签第一行锚点（矩形下边中点再向下偏移 kLabelOffsetMm）
    std::array<double, 3> ComputeLabelAnchor(
        const std::array<double, 3> corners[4],
        ViewType viewType) const;

    /// 根据 RoiStats 生成每行文字内容（6 行）
    static std::vector<std::string> BuildLabelLines(const RoiStats& stats);

    // --- 图元初始化 / 更新 ---
    void InitBorderLines(RoiRecord& roi);
    void UpdateBorderLines(RoiRecord& roi,
        const std::array<double, 3> corners[4]);
    void InitFillActor(RoiRecord& roi,
        const std::array<double, 3> corners[4]);
    void UpdateFillActor(RoiRecord& roi,
        const std::array<double, 3> corners[4]);
    void InitCornerActors(RoiRecord& roi,
        const std::array<double, 3> corners[4]);
    void UpdateCornerActors(RoiRecord& roi,
        const std::array<double, 3> corners[4]);

    vtkSmartPointer<vtkActor> CreateCornerSquare(
        const std::array<double, 3>& center, double halfSize = 2.0);

    /// 全量重绘（移动/缩放后调用）
    void RedrawROI(RoiRecord& roi,
        const std::array<double, 3> corners[4],
        const std::array<double, 3>& anchor);

    void RemoveRoiActors(RoiRecord& roi);

    RoiStats ComputeStats(vtkImageViewer2* viewer,
        const std::array<double, 3>& c1,
        const std::array<double, 3>& c2,
        ViewType viewType, int slice) const;

    bool WorldToScreen(const std::array<double, 3>& world,
        double& outX, double& outY) const;

    int NextId() { return ++m_nextId; }

    // ----------------------------------------------------------------
    //  预览线（懒初始化）
    // ----------------------------------------------------------------
    struct PreviewLine {
        vtkSmartPointer<vtkLineSource>     source;
        vtkSmartPointer<vtkPolyDataMapper> mapper;
        vtkSmartPointer<vtkActor>          actor;
    };
    PreviewLine m_previewLines[4];
    bool        m_previewInitialized = false;

    // ----------------------------------------------------------------
    //  成员变量
    // ----------------------------------------------------------------
    vtkRenderer* m_overlayRenderer = nullptr;
    bool         m_initialized = false;
    bool         m_visible = true;

    std::unordered_map<int, RoiRecord> m_rois;
    int m_nextId = 0;
    int m_lastId = -1;

    bool                  m_isDrawing = false;
    std::array<double, 3> m_drawCorner1 = { 0, 0, 0 };
    ViewType              m_drawView = ViewType::None;
    int                   m_drawSlice = 0;

    static constexpr double kCornerTolerancePx = 8.0;
    static constexpr double kEdgeTolerancePx = 5.0;
    static constexpr double kLabelOffsetMm = 6.0;  ///< 标签第一行距矩形下边的偏移
    static constexpr double kLabelScale = 6.5;  ///< 文字缩放比例
    static constexpr double kLineSpacingMm = 9.0;  ///< 行间距（mm）
};