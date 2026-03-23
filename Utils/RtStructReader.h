#pragma once

#include <string>
#include <vector>
#include <array>

/**
 * @brief 单条轮廓线（对应 RT-S 中一个 ContourSequence 条目）
 */
struct RtContour {
    double sliceZ = 0.0;   ///< 所属切片 Z 坐标（LPS，mm）
    std::vector<std::array<double, 3>> points;   ///< 轮廓点（世界坐标，LPS，mm）
};

/**
 * @brief 一个 ROI（感兴趣区域）
 */
struct RtRoi {
    int         roiNumber = -1;
    std::string roiName;
    std::array<double, 3> color = { 1.0, 0.0, 0.0 };   ///< RGB 0~1
    std::vector<RtContour> contours;
};

/**
 * @brief RT Structure Set 读取工具（DCMTK 实现）
 *
 * DCMTK 对隐式VR/显式VR均透明处理，无需关心传输语法。
 *
 * 依赖库（需在项目中链接）：
 *   dcmdata.lib  ofstd.lib  oflog.lib
 */
class RtStructReader {
public:
    std::vector<RtRoi> Read(const std::string& filePath);

private:
    static std::array<double, 3> ParseColor(const std::string& s);
    static std::vector<std::array<double, 3>> ParseContourData(const std::string& s);
};
