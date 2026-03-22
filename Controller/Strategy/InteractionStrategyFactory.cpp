#include "InteractionStrategyFactory.h"

#include "NormalStrategy.h"
#include "DistanceMeasureStrategy.h"
#include "AngleMeasureStrategy.h"
#include "RegistrationROIStrategy.h"
#include "CheckboardStrategy.h"
//#include "ManualMoveStrategy.h"
#include "ContourMeasureStrategy.h"
#include "FreehandROIStrategy.h"    // 新增
#include "ImageDragStrategy.h"    // 新增
#include "RulerLineStrategy.h"

std::map<InteractionMode, std::unique_ptr<IInteractionStrategy>>
InteractionStrategyFactory::CreateStrategies(IViewController* controller)
{
    std::map<InteractionMode, std::unique_ptr<IInteractionStrategy>> strategies;

    strategies[InteractionMode::Normal]
        = std::make_unique<NormalStrategy>(controller);

    strategies[InteractionMode::DistanceMeasure]
        = std::make_unique<DistanceMeasureStrategy>(controller);

    strategies[InteractionMode::AngleMeasure]
        = std::make_unique<AngleMeasureStrategy>(controller);

    strategies[InteractionMode::RegistrationROI]
        = std::make_unique<RegistrationROIStrategy>(controller);

    strategies[InteractionMode::Checkboard]
        = std::make_unique<CheckboardStrategy>(controller);

    strategies[InteractionMode::ImageDrag]
        = std::make_unique<ImageDragStrategy>(controller);

    strategies[InteractionMode::ContourMeasure]
        = std::make_unique<ContourMeasureStrategy>(controller);

    strategies[InteractionMode::FreehandROI]
        = std::make_unique<FreehandROIStrategy>(controller);

    strategies[InteractionMode::CrosshairRuler]
        = std::make_unique<RulerLineStrategy>(controller);

    return strategies;
}
