#pragma once

#include "RTStructureData.h"
#include <string>
#include <memory>

/// @brief DICOM RT Structure 文件读取器
/// 
/// 使用 GDCM 库解析 DICOM RS 文件，提取所有 ROI 的轮廓数据。
/// 
/// 用法:
///   RtStructReader reader;
///   auto data = reader.Read("path/to/RS.dcm");
///   if (data) {
///       // data->rois 包含所有 ROI 及其轮廓点
///   }
class RtStructReader {
public:
    RtStructReader() = default;
    ~RtStructReader() = default;

    /// @brief 读取 RT Structure 文件
    /// @param filename RS DICOM 文件路径
    /// @return 成功返回 RTStructureData 的智能指针，失败返回 nullptr
    std::shared_ptr<RTStructureData> Read(const std::string& filename);
};