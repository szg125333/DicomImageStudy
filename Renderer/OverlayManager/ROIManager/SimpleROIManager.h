#pragma once

#include "Renderer/OverlayManager/IOverlayFeature.h"
#include "RoiStats.h"

#include <array>
#include <unordered_map>
#include <vtkSmartPointer.h>

class vtkRenderer;
class vtkImageViewer2;
class vtkActor;
class vtkFollower;
class vtkCamera;
class vtkLineSource;
class vtkPolyDataMapper;
class vtkVectorText;

/**
 * @brief ROI 矩形框 Overlay Feature
 *
 * 功能：
 *   - 实时预览拖拽中的半透明矩形框（鼠标按下 → 移动）
 *   - 完成后固定为不透明矩形框，并在右上角显示统计标签
 *   - 统计项：均值、标准差、最小值、最大值、面积、像素数
 *   - 切片切换后，矩形框坐标自动投影到新切片平面
 *   - 支持多个 ROI 同时存在，Delete 键删除最近一个
 *
 * 与 Strategy 的分工：
 *   - Strategy（RegistrationROIStrategy）负责事件分发和状态机
 *   - Manager（本类）负责 VTK 绘制和统计计算，不感知鼠标事件
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
    //  绘制接口（由 Strategy 调用）
    // ----------------------------------------------------------------

    /**
     * @brief 开始一次新的 ROI 绘制
     * @param cornerWorld 鼠标按下点的世界坐标（矩形第一个角点）
     */
    void BeginROI(const std::array<double, 3>& cornerWorld);

    /**
     * @brief 更新预览框（鼠标拖拽过程中实时调用）
     * @param oppositeCornerWorld 鼠标当前位置对应的世界坐标（对角点）
     */
    void UpdatePreview(const std::array<double, 3>& oppositeCornerWorld);

    /**
     * @brief 完成当前 ROI，固定矩形框并计算统计信息
     *
     * @param oppositeCornerWorld 鼠标释放点世界坐标
     * @param viewer              VTK 图像查看器（用于读取像素值）
     * @param viewType            当前视图方向
     * @param slice               当前切片索引
     */
    void CommitROI(const std::array<double, 3>& oppositeCornerWorld,
        vtkImageViewer2* viewer,
        ViewType                      viewType,
        int                           slice);

    /**
     * @brief 取消当前正在绘制的 ROI（右键 / Esc 调用）
     */
    void CancelCurrentROI();

    /**
     * @brief 删除最后一个已完成的 ROI
     */
    void DeleteLastROI();

    /**
     * @brief 删除所有已完成的 ROI 和预览
     */
    void ClearAllROI();

    // ----------------------------------------------------------------
    //  查询接口
    // ----------------------------------------------------------------

    /**
     * @brief 获取最后一次完成的 ROI 统计结果
     * @return 若无有效 ROI 则返回空（IsValid() == false）的 RoiStats
     */
    RoiStats GetLastStats() const;

    /**
     * @brief 获取指定 ROI 的统计结果
     * @param roiId ROI ID
     */
    RoiStats GetStats(int roiId) const;

    /// @brief 当前是否有 ROI 正在绘制（鼠标按下但未释放）
    bool IsDrawing() const { return m_isDrawing; }

private:
    // ----------------------------------------------------------------
    //  ROI 数据结构
    // ----------------------------------------------------------------

    /**
     * @brief 单个 ROI 的完整数据
     *
     * 世界坐标只存两个对角点（corner1 和 corner2），
     * 其余两角通过视图方向推导。
     */
    struct RoiRecord {
        int id = -1;

        /// 矩形的两个对角点（世界坐标）
        std::array<double, 3> corner1 = { 0, 0, 0 };
        std::array<double, 3> corner2 = { 0, 0, 0 };

        /// 绘制时所在的视图方向
        ViewType viewType = ViewType::None;

        /// 绘制时所在的切片索引
        int slice = 0;

        /// 统计结果
        RoiStats stats;

        /// 是否已完成（CommitROI 后为 true）
        bool isComplete = false;

        // ---- VTK 图元 ----
        vtkSmartPointer<vtkActor>    borderActor;  ///< 矩形边框（4 条线段）
        vtkSmartPointer<vtkActor>    fillActor;    ///< 半透明填充面
        vtkSmartPointer<vtkFollower> labelFollower;///< 统计信息悬浮标签
    };

    // ----------------------------------------------------------------
    //  私有方法
    // ----------------------------------------------------------------

    /// @brief 计算4个角点（世界坐标），由两对角点 + 视图方向推导
    void ComputeRectCorners(const std::array<double, 3>& c1,
        const std::array<double, 3>& c2,
        ViewType viewType,
        std::array<double, 3> outCorners[4]) const;

    /// @brief 创建/更新预览框的 4 条边（懒初始化）
    void UpdatePreviewActors(const std::array<double, 3> corners[4]);

    /// @brief 创建固定矩形边框 Actor
    vtkSmartPointer<vtkActor> CreateBorderActor(
        const std::array<double, 3> corners[4], bool isPreview = false);

    /// @brief 创建半透明填充面 Actor
    vtkSmartPointer<vtkActor> CreateFillActor(
        const std::array<double, 3> corners[4]);

    /// @brief 创建统计信息悬浮标签 Follower
    vtkSmartPointer<vtkFollower> CreateStatsLabel(
        const RoiStats& stats,
        const std::array<double, 3>& anchorWorld,
        vtkCamera* camera);

    /// @brief 更新悬浮标签的文字内容（已有 Follower 直接更新，不重建）
    void UpdateStatsLabelText(vtkFollower* follower,
        vtkVectorText* textSrc,
        const RoiStats& stats);

    /**
     * @brief 在图像上计算 ROI 矩形内的像素统计值
     *
     * @param viewer    VTK 图像查看器
     * @param c1        对角点 1（世界坐标）
     * @param c2        对角点 2（世界坐标）
     * @param viewType  视图方向（决定遍历哪两个轴）
     * @param slice     当前切片索引
     * @return 统计结果
     */
    RoiStats ComputeStats(vtkImageViewer2* viewer,
        const std::array<double, 3>& c1,
        const std::array<double, 3>& c2,
        ViewType                      viewType,
        int                           slice) const;

    /// @brief 从渲染器中移除指定 ROI 的所有 Actor
    void RemoveRoiActors(RoiRecord& roi);

    /// @brief 生成下一个 ROI ID
    int NextId() { return ++m_nextId; }

    // ----------------------------------------------------------------
    //  成员变量
    // ----------------------------------------------------------------

    vtkRenderer* m_overlayRenderer = nullptr;
    bool             m_initialized = false;
    bool             m_visible = true;

    /// ROI ID → ROI 数据
    std::unordered_map<int, RoiRecord> m_rois;
    int m_nextId = 0;
    int m_lastId = -1;   ///< 最后一个完成的 ROI ID，用于 DeleteLastROI

    // ---- 当前绘制状态 ----
    bool                   m_isDrawing = false;
    std::array<double, 3>  m_drawCorner1 = { 0, 0, 0 };
    ViewType               m_drawView = ViewType::None;
    int                    m_drawSlice = 0;

    // ---- 预览 Actor（懒初始化，4条边分别用 LineSource）----
    vtkSmartPointer<vtkActor> m_previewBorderActor;

    // 4 条预览线 LineSource，直接更新端点
    struct PreviewLine {
        vtkSmartPointer<vtkLineSource>     source;
        vtkSmartPointer<vtkPolyDataMapper> mapper;
        vtkSmartPointer<vtkActor>          actor;
    };
    PreviewLine m_previewLines[4];   ///< 上/下/左/右 四条边
    bool        m_previewInitialized = false;

    // 预览统计标签
    vtkSmartPointer<vtkVectorText> m_previewLabelText;
    vtkSmartPointer<vtkFollower>   m_previewLabelFollower;
};