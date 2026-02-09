#pragma once
#include <vtkSmartPointer.h>

class vtkImageData;
class vtkRenderer;
class vtkCamera;

enum class SliceOrientation { XY, YZ, XZ };

class IRenderManager {
public:
    virtual ~IRenderManager() = default;

    // 基本渲染输入与方向
    virtual void SetInputData(vtkImageData* img) = 0;
    virtual void SetOrientation(SliceOrientation o) = 0;

    // 切片控制
    virtual void SetSlice(int slice) = 0;
    virtual int GetSlice() const = 0;

    // 窗宽窗位
    virtual void SetWindowLevel(double ww, double wl) = 0;

    // 更新十字线（world/physical 坐标）
    // worldPoint: length 3 array in physical coordinates (mm)
    virtual void UpdateCrosshair(const double worldPoint[3]) = 0;

    // 将 overlay 渲染器的相机与主渲染器相同（用于同步）
    virtual void SetOverlayCamera(vtkCamera* cam) = 0;

    // 获取内部主渲染器（仅用于高级操作或相机同步）
    virtual vtkRenderer* GetRenderer() = 0;
};
