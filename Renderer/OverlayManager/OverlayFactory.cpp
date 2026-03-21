#include "OverlayFactory.h"
#include "SimpleOverlayManager.h"
#include "CrosshairManager/SimpleCrosshairManager.h"
#include "DistanceMeasureManager/SimpleDistanceMeasureManager.h"
#include "AngleMeasureManager/SimpleAngleMeasureManager.h"
#include "OverlayInfoManager/SimpleOverlayInfoManager.h"

std::unique_ptr<IOverlayManager> OverlayFactory::CreateDefault()
{
    auto manager = std::make_unique<SimpleOverlayManager>();

    // ×¢²áË³Ðò¼´äÖÈ¾²ã¼¶Ë³Ðò£¨ÏÈ×¢²áµÄÏÈäÖÈ¾£©
    manager->RegisterFeature(std::make_unique<SimpleCrosshairManager>());
    manager->RegisterFeature(std::make_unique<SimpleDistanceMeasureManager>());
    manager->RegisterFeature(std::make_unique<SimpleAngleMeasureManager>());
    manager->RegisterFeature(std::make_unique<SimpleOverlayInfoManager>());

    return manager;
}
