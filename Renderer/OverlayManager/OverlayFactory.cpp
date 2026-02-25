#include "OverlayFactory.h"
#include "SimpleOverlayManager.h"
#include "CrosshairManager/SimpleCrosshairManager.h"
#include "DistanceMeasureManager/SimpleDistanceMeasureManager.h"

std::unique_ptr<IOverlayManager> CreateDefaultOverlayManager() {
    auto manager = std::make_unique<SimpleOverlayManager>();
    manager->RegisterFeature(std::make_unique<SimpleCrosshairManager>());
    //manager->RegisterFeature(std::make_unique<SimpleDistanceMeasureManager>());
    manager->RegisterFeature(std::make_unique<SimpleDistanceMeasureManager>());
    return manager; // 自动转为 unique_ptr<IOverlayManager>
}