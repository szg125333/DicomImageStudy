#pragma once
#include <QObject>
#include <array>
#include <memory>
#include "Interface/IViewController.h"
#include "Common/InteractionMode.h"
#include "Interface/IViewRenderer.h"
#include "Controller/Strategy/IInteractionStrategy.h"
#include "Common/ViewTypes.h"
#include <optional>

class vtkImageData;

/// @brief 三视图控制器
/// 
/// 管理三个正交视图（轴位、矢状、冠状）的同步和交互：
/// - 切片管理（同步更新三个视图的切片位置）
/// - 交互模式管理（普通模式、切片模式、缩放模式等）
/// - 十字线管理（在三个视图中同步显示）
/// - 窗宽窗位管理（同时应用到所有视图）
class ThreeViewController : public QObject, public IViewController {
    Q_OBJECT
public:
    /// @brief 构造函数
    /// @param parent Qt 父对象
    explicit ThreeViewController(QObject* parent = nullptr);

    /// @brief 析构函数
    ~ThreeViewController() override;

    /// @brief 设置三个视图渲染器
    /// @param renderers 三个 IViewRenderer 指针的数组（Axial/Sagittal/Coronal）
    void SetRenderers(std::array<IViewRenderer*, 3> renderers);

    /// @brief 设置要显示的医学图像数据
    /// @param image VTK 图像数据，包含完整的三维体积数据
    void SetImageData(vtkImageData* image);

    /// @brief 请求更改指定视图的切片位置
    /// @param view 视图类型（Axial/Sagittal/Coronal）
    /// @param slice 新的切片索引
    void RequestSetSlice(ViewType view, int slice);

    /// @brief 获取指定视图的当前切片位置
    /// @param view 视图类型
    /// @return 当前切片索引
    int GetSlice(ViewType view) const;

    /// @brief 设置交互模式
    /// @param mode 交互模式（Normal/Slice/Zoom/WindowLevel）
    void SetInteractionMode(InteractionMode mode);

    /// @brief 获取当前交互模式
    /// @return 当前交互模式
    InteractionMode GetInteractionMode() const { return m_mode; }

    /// @brief 改变指定视图的切片位置（增量）
    /// @param viewIndex 视图索引（0=Axial, 1=Sagittal, 2=Coronal）
    /// @param delta 增量（正为向前，负为向后）
    void ChangeSlice(int viewIndex, int delta) override;

    /// @brief 获取指定视图的渲染器
    /// @param viewIndex 视图索引
    /// @return IViewRenderer 指针
    IViewRenderer* GetRenderer(int viewIndex) override { return m_renderers[viewIndex]; }

    /// @brief 根据点击位置定位
    /// 
    /// 在指定视图上点击时，更新所有视图的切片位置和十字线位置
    /// @param viewIndex 点击的视图索引
    /// @param pos 点击的屏幕坐标（int[2]）
    void LocatePoint(int viewIndex, int* pos) override;

    /// @brief 更新所有视图的十字线位置
    /// @param worldPoint 世界坐标系中的点位置
    void UpdateCrosshairInAllViews(std::array<double, 3> worldPoint);

    /// @brief 设置所有视图的窗宽窗位
    /// @param ww 窗宽（Window Width）
    /// @param wl 窗位（Window Level）
    void SetWindowLevel(double ww, double wl) override;

    /// @brief 获取当前窗宽
    /// @return 窗宽值
    double GetWindowWidth() const override { return m_windowWidth; }

    /// @brief 获取当前窗位
    /// @return 窗位值
    double GetWindowLevel() const override { return m_windowLevel; }

    void OnDistanceMeasurementStart(int viewIndex, int pos[2]) override;
    void OnDistanceMeasurementComplete(int startView, int startPos[2], int endView, int endPos[2]) override;
    void OnDistanceMeasurementCancel() override;
    void OnDistancePreview(int viewIndex, int startPos[2], int currentViewIndex, int currentPos[2]) override;

signals:
    /// @brief 切片改变信号
    /// @param viewIndex 改变的视图索引
    /// @param slice 新的切片索引
    void sliceChanged(int viewIndex, int slice);

private:
    /// @brief 内部更新切片位置（不触发信号）
    /// @param view 视图类型
    /// @param slice 新的切片索引
    void updateSliceInternal(ViewType view, int slice);

    /// @brief 计算每个视图的切片范围
    void computeSliceRanges();

    /// @brief 注册当前交互模式的事件回调
    void registerEvents();

    /// @brief 移除旧交互模式的事件回调
    void unregisterEvents();

    // 工具函数：从屏幕坐标拾取世界坐标
    std::array<double, 3> PickWorldPosition(
        vtkRenderer* renderer,
        int screenX,
        int screenY
    );

private:
    // ==================== 图像数据 ====================
    /// 要显示的医学图像数据
    vtkImageData* m_image = nullptr;

    // ==================== 视图管理 ====================
    /// 三个视图的渲染器（Axial/Sagittal/Coronal）
    std::array<IViewRenderer*, 3> m_renderers;

    /// 每个视图的最小切片索引
    int m_minSlice[3];

    /// 每个视图的最大切片索引
    int m_maxSlice[3];

    /// 防止递归更新切片的标志
    bool m_internalUpdate = false;

    // ==================== 交互管理 ====================
    /// 当前交互模式
    InteractionMode m_mode = InteractionMode::None;

    /// 当前交互策略的实现
    std::unique_ptr<IInteractionStrategy> m_strategy;

    // ==================== 窗宽窗位管理 ====================
    /// 当前窗宽值
    double m_windowWidth = 0.0;

    /// 当前窗位值
    double m_windowLevel = 0.0;
};