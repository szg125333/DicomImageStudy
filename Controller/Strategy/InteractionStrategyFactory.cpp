#include "InteractionStrategyFactory.h"

#include "NormalStrategy.h"
#include "DistanceMeasureStrategy.h"
#include "AngleMeasureStrategy.h"
#include "RegistrationROIStrategy.h"
#include "CheckboardStrategy.h"
#include "ManualMoveStrategy.h"
#include "ContourMeasureStrategy.h"

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

    strategies[InteractionMode::ManualMove]
        = std::make_unique<ManualMoveStrategy>(controller);

    strategies[InteractionMode::ContourMeasure]
        = std::make_unique<ContourMeasureStrategy>(controller);

    return strategies;
}
