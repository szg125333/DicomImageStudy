#pragma once
#include <memory>

class IOverlayManager;

std::unique_ptr<IOverlayManager> CreateDefaultOverlayManager();