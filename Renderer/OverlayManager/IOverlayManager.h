#include "Renderer/OverlayManager/IOverlayFeature.h"
#include <typeinfo>  

class IOverlayManager {
public:
    virtual ~IOverlayManager() = default;

    // 通用控制
    virtual void Initialize(vtkRenderer*, vtkImageViewer2*) = 0;
    virtual void SetVisible(bool visible) = 0;
    virtual void SetColor(double r, double g, double b) = 0;
    virtual void Shutdown() = 0;
    virtual void SetImageWorldBounds(const std::array<double, 6>& bounds) = 0;

    template<typename T>
    T* GetFeature() {
        return dynamic_cast<T*>(GetFeatureImpl(typeid(T)));
    }

protected:
    virtual IOverlayFeature* GetFeatureImpl(const std::type_info& type) = 0;
};
