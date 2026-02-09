#ifndef IMAGE_ORIENTATION_RESAMPLER_H
#define IMAGE_ORIENTATION_RESAMPLER_H

#include "itkImage.h"
#include "itkImageSeriesReader.h"
#include "itkImageSeriesWriter.h"
#include "itkGDCMImageIO.h"
#include "itkMetaDataDictionary.h"

#include <QDir>
#include <QFileInfoList>
#include <QStringList>
#include <QDebug>

#include <vector>
#include <string>
#include <array>
#pragma comment(lib, "Rpcrt4.lib")


class ImageOrientationResampler
{
public:
    using PixelType = short;
    using ImageType = itk::Image<PixelType, 3>;
    using ReaderType = itk::ImageSeriesReader<ImageType>;
    using WriterType = itk::ImageSeriesWriter<ImageType, itk::Image<short, 2>>;
    using ImageIOType = itk::GDCMImageIO;

    ImageOrientationResampler();

    /**
     * @brief 读取 DICOM 序列并返回 ITK 三维图像
     * @param filenames DICOM 文件路径列表
     * @return ITK 三维图像指针
     */
    ImageType::Pointer ReadDicomSeries(const std::vector<std::string>& filenames);

    /**
     * @brief 根据方向矩阵（6 个值：行向量 + 列向量）对图像进行体位变换
     *        流程：ITK → VTK → Reslice → ITK
     * @param inputImage 输入 ITK 图像
     * @param orientation 方向数组（前 3 个为行向量，后 3 个为列向量）
     * @return 变换后的 ITK 图像指针
     */
    ImageType::Pointer ApplyOrientationTransform(ImageType::Pointer inputImage,
        const std::array<double, 6>& orientation);

    /**
     * @brief 将输入图像重采样到目标 CT 图像的几何参数（spacing、origin、方向、大小）
     * @param inputImage 输入图像
     * @param ctImage 目标 CT 图像
     * @return 重采样后的 ITK 图像指针
     */
    ImageType::Pointer ResampleToCT(ImageType::Pointer inputImage, ImageType::Pointer ctImage);

    /**
     * @brief 将 ITK 图像写出为 DICOM 序列
     * @param image 输入图像
     * @param outputFilenames 输出文件路径列表
     */
    void WriteDicomSeries(ImageType::Pointer image,
        const std::vector<std::string>& outputFilenames);

    /**
     * @brief 从指定文件夹加载 DICOM 序列文件列表
     *        自动过滤 RTPLAN 和 RS 文件，并按文件名中的数字排序
     * @param folderPath DICOM 文件夹路径
     * @return DICOM 文件路径列表
     */
    std::vector<std::string> loadDicomSeries(const QString& folderPath);

    /**
     * @brief 根据输入文件列表和输出目录生成新的输出文件名列表
     *        格式：outputDir\image_0000.dcm
     * @param inputFiles 输入文件列表
     * @param outputDir 输出目录
     * @return 输出文件路径列表
     */
    std::vector<std::string> GenerateOutputFileNames(
        const std::vector<std::string>& inputFiles,
        const std::string& outputDir);

    /**
     * @brief 打印 ITK 图像的方向矩阵（3x3）
     * @param image 输入图像
     */
    void PrintDirection(itk::Image<short, 3>::Pointer image);

    /**
     * @brief 打印 ITK 图像的 MetaDataDictionary 中的键值对
     *        如果值为字符串则输出内容，否则标记为非字符串
     * @param dict MetaDataDictionary 对象
     */
    void PrintMetaDataDictionary(const itk::MetaDataDictionary& dict);

    int WriteSlicesUsingNames(itk::Image<short, 3>::Pointer image3D,
        const std::vector<std::string>& outputDicomFiles,const itk::MetaDataDictionary& baseDict);

    std::string GetDicomTag(const std::string& filename,
        const std::string& tag);
    int ExtractTrailingNumber(const std::string& filename);

    std::vector<std::string> SortDicomFiles(const std::vector<std::string>& dicomFiles);
};
#endif // IMAGE_ORIENTATION_RESAMPLER_H
