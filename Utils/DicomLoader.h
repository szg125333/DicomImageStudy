#pragma once

#include <string>
#include <vector>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <itkImage.h>

/// @brief DICOM 图像加载器
///
/// 负责所有 DICOM 图像的加载和预处理工作：
/// - 加载 DICOM 序列 → vtkImageData（自动处理方向矩阵）
/// - 加载 CBCT 并重采样到 CT 网格 → vtkImageData
/// 
/// 调用者不需要关心 ITK/GDCM 的细节。
class DicomLoader {
public:
    using ITKImageType = itk::Image<short, 3>;

    DicomLoader() = default;
    ~DicomLoader() = default;

    /// @brief 加载 DICOM 序列并转换为 vtkImageData
    /// @param dicomFolder DICOM 文件夹路径
    /// @return vtkImageData 指针，失败返回 nullptr
    vtkSmartPointer<vtkImageData> Load(const std::string& dicomFolder);

    /// @brief 加载 CBCT 并重采样到参考图像（CT）的网格上
    /// @param cbctFolder CBCT DICOM 文件夹路径
    /// @param referenceCtFolder CT DICOM 文件夹路径（用于获取目标网格参数）
    /// @return 重采样后的 vtkImageData，与 CT 具有相同的 spacing/origin/dimensions
    vtkSmartPointer<vtkImageData> LoadAndResample(
        const std::string& cbctFolder,
        const std::string& referenceCtFolder);

    /// @brief 加载 CBCT 并重采样到已加载的 CT ITK 图像网格上
    /// @param cbctFolder CBCT DICOM 文件夹路径
    /// @param referenceCtImage 已加载的 CT ITK 图像
    /// @return 重采样后的 vtkImageData
    vtkSmartPointer<vtkImageData> LoadAndResample(
        const std::string& cbctFolder,
        ITKImageType::Pointer referenceCtImage);

private:
    /// @brief 从文件夹加载排序后的 DICOM 文件列表
    std::vector<std::string> loadSortedDicomFiles(const std::string& folder);

    /// @brief 读取 DICOM 序列为 ITK 图像
    ITKImageType::Pointer readDicomAsITK(const std::vector<std::string>& files);

    /// @brief ITK 图像重采样到目标网格
    ITKImageType::Pointer resampleToReference(
        ITKImageType::Pointer inputImage,
        ITKImageType::Pointer referenceImage);

    /// @brief ITK 图像转换为 vtkImageData（包含方向矩阵处理）
    vtkSmartPointer<vtkImageData> itkToVtk(ITKImageType::Pointer itkImage);
};