#pragma once
#include "Interface/IViewRenderer.h"
#include "Common/RenderViewState.h"
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <unordered_map>
#include <functional>
#include <memory>
#include <vtkSmartPointer.h>

class QVTKOpenGLNativeWidget;
class vtkImageViewer2;
class vtkGenericOpenGLRenderWindow;
class vtkCallbackCommand;
class vtkImageData;
class vtkRenderer;
class IOverlayManager;

/// @brief VTK 视图渲染器
/// 
/// 负责管理单个切片视图的渲染，包括：
/// - 图像数据的显示
/// - Overlay 元素的管理（十字线、窗宽窗位信息等）
/// - 用户交互事件的处理
/// - 渲染请求的合并和优化
class VtkViewRenderer : public QObject, public IViewRenderer {
    Q_OBJECT
public:
    /// @brief 构造函数
    /// @param widget Qt VTK 渲染窗口
    VtkViewRenderer(QVTKOpenGLNativeWidget* widget);

    /// @brief 析构函数
    ~VtkViewRenderer() override;

    /// @brief 设置输入图像数据
    /// @param img VTK 图像数据
    void SetInputData(vtkImageData* img) override;

    /// @brief 设置切片方向
    /// @param o 切片方向（XY/YZ/XZ）
    void SetOrientation(ViewType viewType) override;

    /// @brief 设置当前切片索引
    /// @param slice 切片号
    void SetSlice(int slice) override;
    void SetMaxSlice(int slice) override;

    void UpdaBasicInformationActor() override;

    /// @brief 获取当前切片索引
    /// @return 当前切片号
    int GetSlice() const override;

    /// @brief 请求渲染
    /// 
    /// 使用 QTimer 延迟渲染，确保在事件循环中只渲染一次
    /// 即使多个地方同时请求渲染，也只会触发一次 PerformRender()
    void RequestRender() override;

    /// @brief 注册事件回调
    /// @param type 事件类型
    /// @param cb 回调函数，接收位置信息（int[2]）
    void OnEvent(EventType type, std::function<void(const EventData&)> cb);

    /// @brief 获取 VTK 图像查看器
    /// @return vtkImageViewer2 指针
    vtkSmartPointer<vtkImageViewer2> GetViewer() override { return m_viewer; }

    /// @brief 获取 Overlay 渲染器
    /// @return vtkRenderer 指针
    vtkSmartPointer<vtkRenderer> GetOverlayRenderer() override { return m_overlayRenderer; }

    /// @brief 获取 Overlay 管理器
    /// @return IOverlayManager 指针
    IOverlayManager* GetOverlayManager() override { return m_overlayManager.get(); }

    void SetOverlayManager(std::unique_ptr<IOverlayManager> manager) override;

    std::array<double, 3> PickWorldPosition(int screenX, int screenY)override;

    void SetColorWindow(double s) override;
    void SetColorLevel(double s) override;
    void SetCurrentClickWorldPos(std::array<double, 3> worldPos) override;

    double GetColorWindow()  { return m_windowWidth; };
    double GetColorLevel() { return m_windowLevel; };

    int GetCurrentSlice() { return m_currentSlice; };
    int GetAllSlices() { return m_maxSlices; };
    ViewType GetCurrentViewType() { return m_currentViewType; };
    std::array<double, 3> GetCurrentClickWorldPos() { return m_currentClickWorldPos; };
signals:
    void viewStateChanged(const RenderViewState&data);

private slots:
    /// @brief 执行渲染操作（内部使用）
    /// 
    /// 由 QTimer 触发，确保图像和 overlay 都被更新
    void PerformRender();

    void getViewStateChanged(const RenderViewState& data);

private:
    // ==================== Qt 相关 ====================
    /// Qt VTK 渲染窗口
    QPointer<QVTKOpenGLNativeWidget> m_widget;

    /// 渲染计时器，用于延迟渲染合并
    QTimer m_renderTimer;

    // ==================== VTK 相关 ====================
    /// VTK 图像查看器
    vtkSmartPointer<vtkImageViewer2> m_viewer;

    /// VTK 渲染窗口
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;

    /// 用于绘制 overlay 元素的渲染器
    vtkSmartPointer<vtkRenderer> m_overlayRenderer;

    /// VTK 事件回调命令
    vtkSmartPointer<vtkCallbackCommand> m_vtkCmd;

    std::unordered_map<EventType, std::function<void(const EventData&)>> m_callbacks;

    static void VtkGenericCallback(vtkObject* caller, unsigned long eid,
        void* clientdata, void* calldata);

    std::unique_ptr<IOverlayManager> m_overlayManager;

	ViewType m_currentViewType=ViewType::None;

    /// 当前窗宽值
    double m_windowWidth = 0.0;
    double m_windowLevel = 0.0;
	int m_currentSlice = 0;
	int m_maxSlices = 0;
    std::array<double, 3> m_currentClickWorldPos = { 0.0, 0.0, 0.0 };
};