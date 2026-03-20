#pragma once

#include <QObject>
#include <array>
#include <memory>
#include <unordered_map>

#include "Interface/IViewController.h"
#include "Common/InteractionMode.h"
#include "Interface/IViewRenderer.h"
#include "Controller/Strategy/IInteractionStrategy.h"
#include "Common/ViewTypes.h"
#include "Common/EventData.h"

class vtkImageData;

class ThreeViewController : public QObject, public IViewController {
    Q_OBJECT

public:
    explicit ThreeViewController(QObject* parent = nullptr);
    ~ThreeViewController() override;

    void SetRenderers(std::array<IViewRenderer*, 3> renderers);
    void SetImageData(vtkImageData* image);

    void RequestSetSlice(ViewType view, int slice);
    int GetSlice(ViewType view) const;

    void SetInteractionMode(InteractionMode mode);
    InteractionMode GetInteractionMode() const { return m_CurrentMode; }
	void Zoom(int viewIndex, double factor, std::array<double, 3> initialFocalPoint) override;
    void ChangeSlice(int viewIndex, int delta) override;
    IViewRenderer* GetRenderer(int viewIndex) override { return m_renderers[viewIndex]; }

    void UpdateSliceInternals(std::array<double, 3> worldPoint) override;
    const vtkImageData* GetImage() const override;
    void SetWindowLevel(double ww, double wl) override;
    double GetWindowWidth() const override { return m_windowWidth; }
    double GetWindowLevel() const override { return m_windowLevel; }

    std::array<double, 6> GetImageBounds() const;

    void resetStrategyDrawings();   //清除当前绘画

signals:
    void sliceChanged(int viewIndex, int slice);

private:
    void updateSliceInternal(ViewType view, int slice);

    /// @brief 计算每个视图的切片范围
    void computeSliceRanges();

    /// @brief 注册当前交互模式的事件回调
    void registerEvents();

    /// @brief 移除旧交互模式的事件回调
    void unregisterEvents();

private:
    /// 要显示的医学图像数据
    vtkSmartPointer<vtkImageData> m_image;

    /// 三个视图的渲染器（Axial/Sagittal/Coronal）
    std::array<IViewRenderer*, 3> m_renderers;

    /// 每个视图的最小切片索引
    int m_minSlice[3];

    /// 每个视图的最大切片索引
    int m_maxSlice[3];

    /// 防止递归更新切片的标志
    bool m_internalUpdate = false;

    /// 当前交互模式
    InteractionMode m_CurrentMode = InteractionMode::None;

    /// 当前交互策略的实现
    std::unordered_map<InteractionMode, std::unique_ptr<IInteractionStrategy>> m_strategies;

    // ==================== 窗宽窗位管理 ====================
    /// 当前窗宽值
    double m_windowWidth = 0.0;

    /// 当前窗位值
    double m_windowLevel = 0.0;
};