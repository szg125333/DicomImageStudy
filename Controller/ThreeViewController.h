#pragma once

#include "Interface/IViewController.h"
#include "Common/InteractionMode.h"
#include "Common/ViewTypes.h"

#include <QObject>
#include <array>
#include <map>
#include <memory>

#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

class IInteractionStrategy;
class vtkImageData;

/**
 * @brief 三视图控制器
 *
 * 管理轴状（Axial）、矢状（Sagittal）、冠状（Coronal）三个切片视图的
 * 全局状态协调，包括：
 *   - 图像数据绑定与初始化
 *   - 切片同步（点击某视图时其余两个视图对齐）
 *   - 窗宽窗位广播
 *   - 交互模式切换（普通浏览 / 测距 / 测角 / ...）
 *   - 视图缩放
 *
 * 视图渲染器由外部通过 SetRenderers() 注入，控制器不拥有渲染器生命周期。
 */
class ThreeViewController : public QObject, public IViewController {
    Q_OBJECT

public:
    explicit ThreeViewController(QObject* parent = nullptr);
    ~ThreeViewController() override;

    // ----------------------------------------------------------------
    //  初始化
    // ----------------------------------------------------------------

    /**
     * @brief 注入三个视图渲染器
     *
     * 必须在 SetImageData() 之前调用。
     * 顺序：[0]=Axial  [1]=Sagittal  [2]=Coronal
     */
    void SetRenderers(std::array<IViewRenderer*, 3> renderers);

    /**
     * @brief 加载图像数据并初始化三视图
     *
     * 自动计算切片范围、初始化窗宽窗位并将相机设为正交投影。
     */
    void SetImageData(vtkImageData* image);

    // ----------------------------------------------------------------
    //  IViewController 接口实现
    // ----------------------------------------------------------------

    void ChangeSlice(int viewIndex, int delta)                  override;
    void UpdateSliceByWorldPoint(std::array<double, 3> worldPoint) override;
    double GetWindowWidth() const                               override { return m_windowWidth; }
    double GetWindowLevel() const                               override { return m_windowLevel; }
    void SetWindowLevel(double window, double level)            override;
    void Zoom(int viewIndex, double factor,
        std::array<double, 3> focalWorldPoint)            override;
    IViewRenderer* GetRenderer(int viewIndex)                   override;
    const vtkImageData* GetImage() const                        override;

    // ----------------------------------------------------------------
    //  交互模式
    // ----------------------------------------------------------------

    /**
     * @brief 切换交互模式
     *
     * 切换时自动重新注册事件回调，确保新模式立即生效。
     */
    void SetInteractionMode(InteractionMode mode);

    /// @brief 获取当前交互模式
    InteractionMode GetInteractionMode() const { return m_currentMode; }

    // ----------------------------------------------------------------
    //  其他控制
    // ----------------------------------------------------------------

    /// @brief 获取图像世界坐标包围盒 [xMin,xMax, yMin,yMax, zMin,zMax]
    std::array<double, 6> GetImageBounds() const;

    /// @brief 清除当前模式在所有视图上产生的所有 Overlay 标注
    void ClearAllStrategyDrawings();

    // ----------------------------------------------------------------
    //  切片操作（供外部 UI 直接调用）
    // ----------------------------------------------------------------

    /**
     * @brief 请求设置指定视图的切片（含范围夹紧和信号发射）
     * @param view  目标视图方向
     * @param slice 目标切片索引（自动夹紧到有效范围）
     */
    void RequestSetSlice(ViewType view, int slice);

    /// @brief 获取指定视图的当前切片索引
    int GetSlice(ViewType view) const;

signals:
    /**
     * @brief 切片索引发生变化时发出
     * @param viewIndex 视图索引（0–2）
     * @param slice     新切片索引
     */
    void sliceChanged(int viewIndex, int slice);

private:
    // ----------------------------------------------------------------
    //  私有方法
    // ----------------------------------------------------------------

    /// @brief 计算三个方向的切片范围（依赖 m_image 已就绪）
    void ComputeSliceRanges();

    /// @brief 直接设置切片并触发渲染（不发信号，不做边界检查）
    void SetSliceInternal(ViewType view, int slice);

    /// @brief 向三个视图注册事件转发回调
    void RegisterEventCallbacks();

    /// @brief 注销三个视图的事件回调
    void UnregisterEventCallbacks();

    // ----------------------------------------------------------------
    //  成员变量
    // ----------------------------------------------------------------

    std::array<IViewRenderer*, 3> m_renderers = { nullptr, nullptr, nullptr };

    vtkWeakPointer<vtkImageData> m_image;   // 不拥有图像数据生命周期

    double m_windowWidth = 400.0;
    double m_windowLevel = 40.0;

    int m_minSlice[3] = { 0, 0, 0 };
    int m_maxSlice[3] = { 0, 0, 0 };

    InteractionMode m_currentMode = InteractionMode::Normal;

    /// 交互模式 → 策略对象 映射表
    std::map<InteractionMode, std::unique_ptr<IInteractionStrategy>> m_strategies;

    /// 防止 UpdateSliceByWorldPoint 递归触发切片变化
    bool m_isUpdatingSlice = false;
};
