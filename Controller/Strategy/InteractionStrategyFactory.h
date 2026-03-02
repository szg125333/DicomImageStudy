#pragma once

#include <unordered_map>
#include <memory>

class IInteractionStrategy;
class IViewController;
enum class InteractionMode;

class InteractionStrategyFactory {
public:
    using StrategyMap = std::unordered_map<InteractionMode, std::unique_ptr<IInteractionStrategy>>;

    // 禁止实例化（纯静态工具类）
    InteractionStrategyFactory() = delete;

    // 创建所有策略的集合
    static StrategyMap CreateStrategies(IViewController* m_controller);
};