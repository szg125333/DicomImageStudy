#pragma once
#include <functional>
#include <vtkSmartPointer.h> 
#include <vtkImageViewer2.h>
#include "Common/EventData.h"
#include "Common/ViewTypes.h"

class IOverlayManager;
class IOverlayFeature;

//enum class SliceOrientation { XY = 0, YZ = 1, XZ = 2 };

enum class EventType {
    WheelForward,
    WheelBackward,
    LeftPress,
    LeftMove,
    LeftRelease,
    RightPress,
    RightMove,
    RightRelease,
    KeyPress,      // 按下任意键
    KeyRelease    // 释放任意键（可选，多数场景只需 KeyPress）
};


class IViewRenderer {
public:
    virtual ~IViewRenderer() = default;

    // 数据与视图设置
    virtual void SetInputData(vtkImageData* img) = 0;
    virtual void SetOrientation(ViewType viewType) = 0;
    virtual void SetSlice(int slice) = 0;
    virtual void SetMaxSlice(int slice) = 0;
    virtual int GetSlice() const = 0;

    // 窗宽窗位
    virtual void SetColorWindow(double s) = 0;
    virtual void SetColorLevel(double s) = 0;

    // 交互与事件
    virtual void SetCurrentClickWorldPos(std::array<double, 3> worldPos) = 0;
    virtual std::array<double, 3> PickWorldPosition(int screenX, int screenY) = 0;
    virtual void OnEvent(EventType type, std::function<void(const EventData&)> cb) = 0;

    // 渲染控制
    virtual void RequestRender() = 0;

    // Viewer 与 Overlay 访问
    virtual vtkSmartPointer<vtkImageViewer2> GetViewer() = 0;
    virtual IOverlayManager* GetOverlayManager() = 0;
    virtual vtkSmartPointer<vtkRenderer> GetOverlayRenderer() = 0;      // 返回 overlay 层
    virtual void SetOverlayManager(std::unique_ptr<IOverlayManager> manager) = 0;

    // UI 更新（保留原始拼写）
    virtual void UpdaBasicInformationActor() = 0;
};