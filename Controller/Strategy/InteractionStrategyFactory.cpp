#include "InteractionStrategyFactory.h"
#include "IInteractionStrategy.h"

#include "Controller/Strategy/NormalStrategy.h"
#include "Controller/Strategy/DistanceMeasureStrategy.h"
#include "Controller/Strategy/AngleMeasureStrategy.h"

InteractionStrategyFactory::StrategyMap
InteractionStrategyFactory::CreateStrategies(IViewController* m_controller) {
    StrategyMap strategies;

    // 按需注册已实现的策略
    strategies[InteractionMode::Normal] =
        std::make_unique<NormalStrategy>(m_controller);

    strategies[InteractionMode::DistanceMeasure] =
        std::make_unique<DistanceMeasureStrategy>(m_controller);

     strategies[InteractionMode::AngleMeasure] = 
         std::make_unique<AngleMeasureStrategy>(m_controller);

    // 未实现的模式可以跳过（不加入 map），或用空指针占位（不推荐）

    return strategies;
}