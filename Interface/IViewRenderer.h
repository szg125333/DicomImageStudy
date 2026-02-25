#pragma once
#include <functional>
#include <vtkSmartPointer.h> 
#include <vtkImageViewer2.h>

class IOverlayManager;
class IOverlayFeature;

enum class SliceOrientation { XY = 0, YZ = 1, XZ = 2 };

enum class EventType {
    WheelForward,
    WheelBackward,
    LeftPress,
    LeftMove,
    LeftRelease,
    RightPress,
    RightMove,
    RightRelease
};


class IViewRenderer {
public:
    virtual ~IViewRenderer() = default;
    virtual void SetInputData(vtkImageData* img) = 0;
    virtual void SetOrientation(SliceOrientation o) = 0;
    virtual void SetSlice(int slice) = 0;
    virtual int GetSlice() const = 0;
    virtual void RequestRender() = 0;
    virtual void OnEvent(EventType type, std::function<void(void*)> cb) = 0;
    virtual std::array<double, 3> PickWorldPosition(int screenX, int screenY) = 0;

    virtual vtkSmartPointer<vtkImageViewer2> GetViewer() = 0;
    virtual IOverlayManager* GetOverlayManager()=0;
    virtual vtkSmartPointer<vtkRenderer> GetOverlayRenderer() = 0;      // ·µ»Ø overlay ²ã
    virtual void SetOverlayManager(std::unique_ptr<IOverlayManager> manager)=0;

};
