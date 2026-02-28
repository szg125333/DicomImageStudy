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
    virtual bool isWorldPosInValidPixel(const std::array<double, 3>& worldPos) = 0;
};