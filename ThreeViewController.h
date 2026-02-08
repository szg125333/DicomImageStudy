#pragma once
#include <QObject>
#include <array>
#include <memory>
#include "IViewController.h"
#include "InteractionMode.h"
#include "IViewRenderer.h"
#include "IInteractionStrategy.h"

class vtkImageData;
class vtkTextActor;
class vtkActor2D;
class vtkActor;
class vtkLineSource;

class ThreeViewController : public QObject, public IViewController {
    Q_OBJECT
public:
    enum ViewType { Axial = 0, Sagittal = 1, Coronal = 2 };
    explicit ThreeViewController(QObject* parent = nullptr);

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
    void UpdateCrosshairInAllViews();
    void SetWindowLevel(double ww, double wl)override;
    void SetWindowWidth(double ww) { m_windowWidth = ww; } 
    void SetWindowLevel(double wl) { m_windowLevel = wl; } 
    double GetWindowWidth() const override { return m_windowWidth; }
    double GetWindowLevel() const override { return m_windowLevel; }
signals:
    void sliceChanged(int viewIndex, int slice);

private:
    void updateSliceInternal(ViewType view, int slice);
    void syncFrom(ViewType srcView, int srcSlice);
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

    double m_crossPoint[4] = { 0,0,0,0 }; // x, y, z 三个方向的交叉点坐标

    std::array<vtkSmartPointer<vtkTextActor>, 3> m_textActors;

    std::array<double, 2> m_scalarRange{ 0.0, 0.0 }; // 保存图像灰度范围
    double m_windowWidth = 400; // 默认初始值
    double m_windowLevel = 40; // 默认初始值

private:
    std::array<vtkSmartPointer<vtkActor2D>, 3> m_crossActorsH; // 水平线
    std::array<vtkSmartPointer<vtkActor2D>, 3> m_crossActorsV; // 垂直线
    std::array<vtkSmartPointer<vtkLineSource>, 3> m_crossLinesH; // 水平线源
    std::array<vtkSmartPointer<vtkLineSource>, 3> m_crossLinesV; // 垂直线源

    // 新增：3D 十字线用的 actor / line source / mapper
    std::array<vtkSmartPointer<vtkActor>, 3> m_crossActorsH3D;
    std::array<vtkSmartPointer<vtkActor>, 3> m_crossActorsV3D;
    std::array<vtkSmartPointer<vtkLineSource>, 3> m_crossLinesH3D;
    std::array<vtkSmartPointer<vtkLineSource>, 3> m_crossLinesV3D;
};
