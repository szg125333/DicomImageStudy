#pragma once
#include <QObject>
#include <array>
#include <memory>
#include "InteractionMode.h"
#include "IViewRenderer.h"
#include "IInteractionStrategy.h"

class vtkImageData;

class ThreeViewController : public QObject {
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

signals:
    void sliceChanged(int viewIndex, int slice);

private:
    void updateSliceInternal(ViewType view, int slice);
    void syncFrom(ViewType srcView, int srcSlice);
    void computeSliceRanges();

private:
    vtkImageData* m_image = nullptr;
    std::array<IViewRenderer*, 3> m_renderers;
    int m_minSlice[3];
    int m_maxSlice[3];
    bool m_internalUpdate = false;

    InteractionMode m_mode = InteractionMode::Normal;
    std::unique_ptr<IInteractionStrategy> m_strategy;
};
