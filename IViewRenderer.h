#pragma once
#include <functional>

class vtkImageData;

enum class SliceOrientation { XY = 0, YZ = 1, XZ = 2 };

enum class EventType {
    WheelForward,
    WheelBackward,
    LeftClick,
    RightClick,
    // 可以继续扩展更多事件
};

class IViewRenderer {
public:
    virtual ~IViewRenderer() = default;
    virtual void SetInputData(vtkImageData* img) = 0;
    virtual void SetOrientation(SliceOrientation o) = 0;
    virtual void SetSlice(int slice) = 0;
    virtual int GetSlice() const = 0;
    virtual void Render() = 0;
    virtual void OnEvent(EventType type, std::function<void()> cb) = 0;
};
