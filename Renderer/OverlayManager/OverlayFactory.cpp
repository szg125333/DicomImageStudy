#include "OverlayFactory.h"
#include "SimpleOverlayManager.h"
#include "CrosshairManager/SimpleCrosshairManager.h"
#include "DistanceMeasureManager/SimpleDistanceMeasureManager.h"
#include "OverlayInfoManager/SimpleOverlayInfoManager.h"
#include "AngleMeasureManager/SimpleAngleMeasureManager.h"

std::unique_ptr<IOverlayManager> OverlayFactory::CreateDefault() {
    auto manager = std::make_unique<SimpleOverlayManager>();
    manager->RegisterFeature(std::make_unique<SimpleCrosshairManager>());
    manager->RegisterFeature(std::make_unique<SimpleDistanceMeasureManager>());
    manager->RegisterFeature(std::make_unique<SimpleOverlayInfoManager>());
    manager->RegisterFeature(std::make_unique<SimpleAngleMeasureManager>());
    return manager;
}