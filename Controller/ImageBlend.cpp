#include "ImageBlend.h"
#include <vtkImageCast.h>
#include <iostream>

ImageBlend::ImageBlend() {
    m_blender = vtkSmartPointer<vtkImageBlend>::New();
    m_blender->SetBlendModeToNormal();
}

void ImageBlend::SetPrimaryImage(vtkImageData* image) {
    if (!image) return;
    m_primary = image;
    m_dirty = true;
}

void ImageBlend::SetSecondaryImage(vtkImageData* image) {
    if (!image) return;

    // 验证两个图像的尺寸是否一致
    if (m_primary) {
        int dimsPri[3], dimsSec[3];
        m_primary->GetDimensions(dimsPri);
        image->GetDimensions(dimsSec);

        if (dimsPri[0] != dimsSec[0] || dimsPri[1] != dimsSec[1] || dimsPri[2] != dimsSec[2]) {
            std::cerr << "[ImageBlend] WARNING: Image dimensions mismatch!"
                << " CT: " << dimsPri[0] << "x" << dimsPri[1] << "x" << dimsPri[2]
                << " CBCT: " << dimsSec[0] << "x" << dimsSec[1] << "x" << dimsSec[2]
                << ". CBCT must be resampled to CT grid first." << std::endl;
            return;
        }
    }

    m_secondary = image;
    m_dirty = true;
}

void ImageBlend::SetBlendRatio(double ratio) {
    ratio = std::max(0.0, std::min(1.0, ratio));
    if (std::abs(ratio - m_ratio) < 1e-6) return;

    m_ratio = ratio;
    m_dirty = true;
}

bool ImageBlend::IsReady() const {
    return m_primary != nullptr && m_secondary != nullptr;
}

vtkImageData* ImageBlend::GetBlendedImage() {
    if (m_dirty) {
        updateBlend();
        m_dirty = false;
    }

    // 只有 CT，没有 CBCT → 返回纯 CT
    if (!m_secondary) return m_primary;

    // ratio == 0 → 纯 CT，跳过 blend 节省性能
    if (m_ratio < 1e-6) return m_primary;

    // ratio == 1 → 纯 CBCT
    if (m_ratio > 1.0 - 1e-6) return m_secondary;

    return m_blender->GetOutput();
}

void ImageBlend::updateBlend() {
    if (!m_primary) return;

    // 只有一个图像，不需要 blend
    if (!m_secondary) return;
    if (m_ratio < 1e-6 || m_ratio > 1.0 - 1e-6) return;

    // vtkImageBlend 需要相同的标量类型
    // CT 和 CBCT 通常都是 short，但以防万一做一次类型转换
    auto castPrimary = vtkSmartPointer<vtkImageCast>::New();
    castPrimary->SetInputData(m_primary);
    castPrimary->SetOutputScalarTypeToDouble();
    castPrimary->Update();

    auto castSecondary = vtkSmartPointer<vtkImageCast>::New();
    castSecondary->SetInputData(m_secondary);
    castSecondary->SetOutputScalarTypeToDouble();
    castSecondary->Update();

    m_blender->RemoveAllInputs();
    m_blender->AddInputConnection(castPrimary->GetOutputPort());
    m_blender->AddInputConnection(castSecondary->GetOutputPort());

    // 设置权重：CT 权重 = 1 - ratio, CBCT 权重 = ratio
    m_blender->SetOpacity(0, 1.0 - m_ratio);
    m_blender->SetOpacity(1, m_ratio);

    m_blender->Update();
}