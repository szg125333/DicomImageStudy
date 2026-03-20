// IViewController.h
#pragma once

class IViewRenderer;
class vtkImageData;

class IViewController {
public:
    virtual ~IViewController() = default;

    virtual void Zoom(int viewIndex, double factor, std::array<double, 3> initialFocalPoint) = 0;
    virtual void ChangeSlice(int viewIndex, int delta) = 0;
    virtual double GetWindowWidth() const = 0;
    virtual double GetWindowLevel() const = 0;
    virtual void SetWindowLevel(double window, double level) = 0;
    virtual void UpdateSliceInternals(std::array<double, 3> worldPoint) = 0;
    virtual IViewRenderer* GetRenderer(int viewIndex) = 0;
    virtual const vtkImageData* GetImage() const = 0;
};