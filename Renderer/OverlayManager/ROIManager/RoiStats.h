#pragma once

#include <array>

/**
 * @brief ROI 区域统计结果
 *
 * 存储在矩形 ROI 框内对图像像素（HU 值）的统计信息。
 * 由 SimpleROIManager::ComputeStats() 计算并缓存，
 * 外部通过 GetStats() 获取后显示在信息面板或悬浮标签上。
 */
struct RoiStats {
    double mean = 0.0;   ///< 均值（Mean）
    double stdDev = 0.0;   ///< 标准差（Std Dev）
    double minVal = 0.0;   ///< 最小值（Min）
    double maxVal = 0.0;   ///< 最大值（Max）
    double area = 0.0;   ///< 面积（mm²，由 spacing 换算）
    int    pixelCount = 0;  ///< 参与统计的像素数量

    /// @brief 统计结果是否有效（像素数量 > 0）
    bool IsValid() const { return pixelCount > 0; }
};