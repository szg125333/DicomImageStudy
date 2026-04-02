#pragma once

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkImageBlend.h>

/// @brief 双图像混合控制器
///
/// 原理：
/// 持有两个独立的 vtkImageData（CT 和 CBCT），
/// 用 vtkImageBlend 按权重混合输出一个 vtkImageData。
/// 
/// 要求：两个图像必须具有相同的 dimensions、spacing、origin。
/// 如果 CBCT 与 CT 不一致，需要先用 ImageOrientationResampler::ResampleToCT()
/// 将 CBCT 重采样到 CT 的网格上。
///
/// 混合公式：output = (1 - ratio) * CT + ratio * CBCT
/// ratio = 0.0 → 纯 CT
/// ratio = 1.0 → 纯 CBCT
/// ratio = 0.5 → 半透明叠加
class ImageBlend {
public:
    ImageBlend();
    ~ImageBlend() = default;

    /// @brief 设置基准图像（CT）
    void SetPrimaryImage(vtkImageData* image);

    /// @brief 设置叠加图像（CBCT，必须已重采样到与 CT 一致的网格）
    void SetSecondaryImage(vtkImageData* image);

    /// @brief 设置混合比例
    /// @param ratio 0.0 = 纯 CT, 1.0 = 纯 CBCT
    void SetBlendRatio(double ratio);

    /// @brief 获取当前混合比例
    double GetBlendRatio() const { return m_ratio; }

    /// @brief 获取混合后的输出图像
    /// @return 混合后的 vtkImageData，可直接传给 SetImageData
    vtkImageData* GetBlendedImage();

    /// @brief 是否已经设置了两个图像
    bool IsReady() const;

private:
    void updateBlend();

    vtkSmartPointer<vtkImageData> m_primary;    // CT
    vtkSmartPointer<vtkImageData> m_secondary;  // CBCT（已重采样）
    vtkSmartPointer<vtkImageBlend> m_blender;
    double m_ratio = 0.5;  // 默认纯 CT
    bool m_dirty = true;
};