#pragma once

#include <memory>

class IOverlayManager;

class OverlayFactory {
public:
    // 禁止实例化（纯工具类）
    OverlayFactory() = delete;
    ~OverlayFactory() = delete;

    /// @brief 创建默认的 OverlayManager（带标准功能：十字线、测距、信息显示）
    static std::unique_ptr<IOverlayManager> CreateDefault();
};