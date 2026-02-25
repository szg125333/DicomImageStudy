// IViewController.h
#pragma once

class IViewRenderer;

class IViewController {
public:
    virtual ~IViewController() = default;

    virtual void ChangeSlice(int viewIndex, int delta) = 0;
    virtual double GetWindowWidth() const = 0;
    virtual double GetWindowLevel() const = 0;
    virtual void SetWindowLevel(double window, double level) = 0;
    virtual void LocatePoint(int viewIndex, int pos[2]) = 0;
    virtual IViewRenderer* GetRenderer(int viewIndex) = 0;

    virtual void OnDistanceMeasurementStart(int viewIndex, int pos[2]) = 0;     // 开始距离测量（可选，用于高亮十字线等）
    virtual void OnDistanceMeasurementComplete(int startView, int startPos[2], int endView, int endPos[2]) = 0;    // 完成一次距离测量（核心！）
    virtual void OnDistanceMeasurementCancel() = 0;    // 取消测量
    virtual void OnDistancePreview(int viewIndex, int startPoint[2], int currentViewIndex, int currentPoint[2]) = 0;
};