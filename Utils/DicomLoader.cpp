#include "DicomLoader.h"

// ITK
#include <itkGDCMSeriesFileNames.h>
#include <itkGDCMImageIO.h>
#include <itkImageSeriesReader.h>
#include <itkOrientImageFilter.h>
#include <itkImageToVTKImageFilter.h>
#include <itkImage.h>

// VTK
#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <vtkNew.h>

#include <QDebug>
#include <algorithm>

using PixelType = short;
using ImageType = itk::Image<PixelType, 3>;

// ============================================================
//  查找 DICOM 文件
// ============================================================

std::vector<std::string> DicomLoader::FindDicomFiles(const std::string& folderPath)
{
    // 用 ITK GDCM 自动发现并排序所有 DICOM 切片
    auto nameGenerator = itk::GDCMSeriesFileNames::New();
    nameGenerator->SetUseSeriesDetails(true);
    nameGenerator->SetDirectory(folderPath);

    const auto& seriesUIDs = nameGenerator->GetSeriesUIDs();
    if (seriesUIDs.empty()) {
        qWarning() << "[DicomLoader] 未发现 DICOM series：" << QString::fromStdString(folderPath);
        return {};
    }

    // 多个 series 时取切片数最多的（通常是主 CT 序列）
    std::string bestUID;
    size_t maxFiles = 0;
    for (const auto& uid : seriesUIDs) {
        auto files = nameGenerator->GetFileNames(uid);
        if (files.size() > maxFiles) {
            maxFiles = files.size();
            bestUID = uid;
        }
    }

    auto files = nameGenerator->GetFileNames(bestUID);
    qDebug() << "[DicomLoader] 选取 series:" << QString::fromStdString(bestUID)
        << "切片数:" << files.size();
    return files;
}

// ============================================================
//  主加载函数
// ============================================================

vtkSmartPointer<vtkImageData> DicomLoader::Load(const std::string& folderPath)
{
    // ── Step 1：发现并排序 DICOM 文件 ────────────────────────────
    auto files = FindDicomFiles(folderPath);
    if (files.empty()) return nullptr;

    // ── Step 2：ITK 读取 DICOM 序列 ──────────────────────────────
    auto dicomIO = itk::GDCMImageIO::New();
    auto reader = itk::ImageSeriesReader<ImageType>::New();
    reader->SetImageIO(dicomIO);
    reader->SetFileNames(files);
    // 强制按 Image Position Patient 排序（而不是文件名顺序）
    reader->ForceOrthogonalDirectionOff();

    try {
        reader->Update();
    }
    catch (const itk::ExceptionObject& ex) {
        qWarning() << "[DicomLoader] ITK 读取失败：" << ex.what();
        return nullptr;
    }

    ImageType::Pointer itkImage = reader->GetOutput();

    // ── 打印读取到的图像几何信息 ──────────────────────────────────
    {
        const auto& o = itkImage->GetOrigin();
        const auto& s = itkImage->GetSpacing();
        const auto& d = itkImage->GetDirection();
        const auto  sz = itkImage->GetLargestPossibleRegion().GetSize();

        qDebug() << "[DicomLoader] ITK origin :"
            << o[0] << o[1] << o[2];
        qDebug() << "[DicomLoader] ITK spacing:"
            << s[0] << s[1] << s[2];
        qDebug() << "[DicomLoader] ITK size   :"
            << sz[0] << sz[1] << sz[2];
        qDebug() << "[DicomLoader] Direction  :"
            << d[0][0] << d[0][1] << d[0][2]
            << "|" << d[1][0] << d[1][1] << d[1][2]
            << "|" << d[2][0] << d[2][1] << d[2][2];
    }

    // ── Step 3：用 OrientImageFilter 重定向到 LPS 标准轴对齐 ─────

    //using OrientFilterType = itk::OrientImageFilter<ImageType, ImageType>;
    //auto orientFilter = OrientFilterType::New();
    //orientFilter->SetInput(itkImage);
    //orientFilter->UseImageDirectionOn();
    //orientFilter->SetDesiredCoordinateOrientation(
    //    itk::SpatialOrientationEnums::ValidCoordinateOrientations::ITK_COORDINATE_ORIENTATION_RAS);

    //try {
    //    orientFilter->Update();
    //}
    //catch (const itk::ExceptionObject& ex) {
    //    qWarning() << "[DicomLoader] OrientImageFilter 失败：" << ex.what();
    //    return nullptr;
    //}

    //ImageType::Pointer orientedImage = orientFilter->GetOutput();

    //// ── 打印重定向后的几何信息 ────────────────────────────────────
    //{
    //    const auto& o = orientedImage->GetOrigin();
    //    const auto& s = orientedImage->GetSpacing();
    //    const auto  sz = orientedImage->GetLargestPossibleRegion().GetSize();

    //    qDebug() << "[DicomLoader] 重定向后 origin :" << o[0] << o[1] << o[2];
    //    qDebug() << "[DicomLoader] 重定向后 spacing:" << s[0] << s[1] << s[2];
    //    qDebug() << "[DicomLoader] 重定向后 size   :" << sz[0] << sz[1] << sz[2];

    //    m_origin = { o[0], o[1], o[2] };
    //    m_spacing = { s[0], s[1], s[2] };
    //}

    // ── Step 4：ITK → VTK 转换 ───────────────────────────────────
    using ConnectorType = itk::ImageToVTKImageFilter<ImageType>;
    auto connector = ConnectorType::New();
    //connector->SetInput(orientedImage);
    connector->SetInput(itkImage);

    try {
        connector->Update();
    }
    catch (const itk::ExceptionObject& ex) {
        qWarning() << "[DicomLoader] ITK→VTK 转换失败：" << ex.what();
        return nullptr;
    }

    // Deep copy，避免 ITK pipeline 被销毁后 VTK 数据失效
    auto vtkImage = vtkSmartPointer<vtkImageData>::New();
    vtkImage->DeepCopy(connector->GetOutput());

    // ── Step 5：把 LPS origin 设置到 vtkImageData ─────────────────
    //
    // 关键：itk::ImageToVTKImageFilter 不传递 origin，
    // 转换后 vtkImageData 的 origin 默认是 (0,0,0)。
    // 必须手动把 ITK origin 设置回去，才能使 VTK 世界坐标与 LPS 对齐。
    //
    // 这一步是解决"CT 坐标系与 RT-S 坐标系不对齐"的核心修复。
    //
    const auto& o = itkImage->GetOrigin();
    const auto& s = itkImage->GetSpacing();
    vtkImage->SetOrigin(o[0], o[1], o[2]);
    vtkImage->SetSpacing(s[0], s[1], s[2]);

    qDebug() << "[DicomLoader] vtkImageData origin :"
        << o[0] << o[1] << o[2];
    qDebug() << "[DicomLoader] vtkImageData spacing:"
        << s[0] << s[1] << s[2];

    // ── 验证：bounds 应该与轮廓坐标范围一致 ──────────────────────
    double bounds[6];
    vtkImage->GetBounds(bounds);
    qDebug() << "[DicomLoader] bounds X:" << bounds[0] << "~" << bounds[1];
    qDebug() << "[DicomLoader] bounds Y:" << bounds[2] << "~" << bounds[3];
    qDebug() << "[DicomLoader] bounds Z:" << bounds[4] << "~" << bounds[5];

    return vtkImage;
}
