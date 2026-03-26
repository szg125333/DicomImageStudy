#pragma once

#include <string>
#include <vector>
#include <array>
#include <vtkSmartPointer.h>

class vtkImageData;

/**
 * @brief 标准 DICOM CT 序列加载器
 *
 * 设计原则：
 *   加载完成后，vtkImageData 的世界坐标（origin + spacing）
 *   与 DICOM LPS 物理坐标完全对齐，无需任何额外变换。
 *   这样 RT-S 轮廓点（LPS 坐标，mm）可以直接叠加显示，无需坐标转换。
 *
 * 核心流程（正确做法）：
 *   1. ITK 读取 DICOM 序列（保留 LPS origin/spacing/direction）
 *   2. 用 itkOrientImageFilter 把图像重定向为 LPS 标准轴对齐（RAI→LPS）
 *      这一步只是重新排列体素顺序和翻转轴，不做插值，不改变物理坐标系
 *   3. ITK → VTK 转换（itk::ImageToVTKImageFilter）
 *   4. 把 ITK 的 LPS origin 手动设置到 vtkImageData
 *      （因为 ITK→VTK 转换器不传递 origin）
 *
 * 错误做法（之前的问题）：
 *   用 vtkImageReslice 对图像做空间变换 → origin 被重置 → 坐标系漂移
 *
 * 加载结果验证：
 *   vtkImageData::GetOrigin() 应该等于 DICOM Image Position Patient (0020,0032)
 *   vtkImageData::GetSpacing() 应该等于 DICOM Pixel Spacing + Slice Thickness
 */
class DicomLoader {
public:
    DicomLoader() = default;

    /**
     * @brief 从文件夹加载 CT DICOM 序列
     *
     * @param folderPath  DICOM 文件夹路径
     * @return            vtkImageData（LPS 坐标系，origin 与 DICOM 对齐）
     *                    失败返回 nullptr
     */
    vtkSmartPointer<vtkImageData> Load(const std::string& folderPath);

    /**
     * @brief 获取加载后图像的 LPS origin（供外部验证使用）
     */
    std::array<double, 3> GetOrigin() const { return m_origin; }

    /**
     * @brief 获取加载后图像的 spacing
     */
    std::array<double, 3> GetSpacing() const { return m_spacing; }

private:
    std::vector<std::string> FindDicomFiles(const std::string& folderPath);

    std::array<double, 3> m_origin = { 0, 0, 0 };
    std::array<double, 3> m_spacing = { 1, 1, 1 };
};
