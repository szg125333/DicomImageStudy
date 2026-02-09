#pragma once
#include <QObject>
#include <array>
#include <memory>
#include "IViewController.h"
#include "InteractionMode.h"
#include "IViewRenderer.h"
#include "IInteractionStrategy.h"
#include "ICrosshairManager.h"
#include "IWindowLevelManager.h"
#include "ViewTypes.h"
class vtkImageData;
class vtkTextActor;
class vtkActor2D;
class vtkActor;
class vtkLineSource;

class ThreeViewController : public QObject, public IViewController {
    Q_OBJECT
public:
    explicit ThreeViewController(QObject* parent = nullptr);
    ~ThreeViewController() override; // 确保声明了析构函数

    void SetRenderers(std::array<IViewRenderer*, 3> renderers);
    void SetImageData(vtkImageData* image);

    void RequestSetSlice(ViewType view, int slice);
    int GetSlice(ViewType view) const;

    void SetInteractionMode(InteractionMode mode);
    InteractionMode GetInteractionMode() const { return m_mode; }
    
    void ChangeSlice(int viewIndex, int delta)override;
    IViewRenderer* GetRenderer(int viewIndex) { return m_renderers[viewIndex]; }
    void LocatePoint(int viewIndex, int* pos)override;
    void syncCameras();

    // 更新交叉点到所有视图 
    void UpdateCrosshairInAllViews(std::array<double, 3> worldPoint);
    void SetWindowLevel(double ww, double wl)override;
    void SetWindowWidth(double ww) { m_windowWidth = ww; } 
    void SetWindowLevel(double wl) { m_windowLevel = wl; } 
    double GetWindowWidth() const override { return m_windowWidth; }
    double GetWindowLevel() const override { return m_windowLevel; }
signals:
    void sliceChanged(int viewIndex, int slice);

private:
    void updateSliceInternal(ViewType view, int slice);
    void computeSliceRanges();
    void registerEvents();   // 注册当前模式的事件
    void unregisterEvents(); // 移除旧模式的事件

private:
    vtkImageData* m_image = nullptr;
    std::array<IViewRenderer*, 3> m_renderers;
    int m_minSlice[3];
    int m_maxSlice[3];
	bool m_internalUpdate = false;  //防止递归更新切片

    InteractionMode m_mode = InteractionMode::None;
    std::unique_ptr<IInteractionStrategy> m_strategy;

    double m_windowWidth; // 默认初始值
    double m_windowLevel; // 默认初始值
};
