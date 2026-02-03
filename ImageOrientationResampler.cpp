#include "ImageOrientationResampler.h"

#include "itkResampleImageFilter.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkImageToVTKImageFilter.h"
#include "itkVTKImageToImageFilter.h"
#include "itkImageDuplicator.h"

#include <vtkImageReslice.h>
#include <vtkMatrix4x4.h>
#include <vtkSmartPointer.h>
#include <vtkMath.h>
#include <vtkMetaImageWriter.h>

#include <sstream>
#include <iomanip>
#include <iostream>
#include <gdcmUIDGenerator.h>
#include "itkExtractImageFilter.h"
#include <regex>
ImageOrientationResampler::ImageOrientationResampler() {}

ImageOrientationResampler::ImageType::Pointer

ImageOrientationResampler::ReadDicomSeries(const std::vector<std::string>& filenames)
{
    auto reader = ReaderType::New();
    auto dicomIO = ImageIOType::New();
    reader->SetImageIO(dicomIO);
    reader->SetFileNames(filenames);
    reader->Update();
    return reader->GetOutput();
}

ImageOrientationResampler::ImageType::Pointer
ImageOrientationResampler::ApplyOrientationTransform(ImageType::Pointer inputImage,
    const std::array<double, 6>& orientation)
{
    using Image3D = itk::Image<PixelType, 3>;

    Image3D::SpacingType spacing = inputImage->GetSpacing();

    using ITKToVTKFilterType = itk::ImageToVTKImageFilter<ImageType>;
    auto itkToVtk = ITKToVTKFilterType::New();
    itkToVtk->SetInput(inputImage);
    itkToVtk->Update();

    vtkSmartPointer<vtkImageData> vtkImg = vtkSmartPointer<vtkImageData>::New();
    vtkImg->DeepCopy(itkToVtk->GetOutput());

    double row[3] = { orientation[0], orientation[1], orientation[2] };
    double col[3] = { orientation[3], orientation[4], orientation[5] };
    vtkMath::Normalize(row);
    vtkMath::Normalize(col);

    double normal[3];
    vtkMath::Cross(row, col, normal);
    vtkMath::Normalize(normal);

    vtkSmartPointer<vtkMatrix4x4> axes = vtkSmartPointer<vtkMatrix4x4>::New();
    axes->Identity();
    for (int i = 0; i < 3; i++)
    {
        axes->SetElement(i, 0, row[i]);
        axes->SetElement(i, 1, col[i]);
        axes->SetElement(i, 2, normal[i]);
    }

    vtkSmartPointer<vtkMatrix4x4> invAxes = vtkSmartPointer<vtkMatrix4x4>::New();
    //invAxes->Identity();
    invAxes->DeepCopy(axes);
    invAxes->Invert();   // 关键：反变换

    vtkNew<vtkImageReslice> reslice;
    reslice->SetInputData(vtkImg);
    //reslice->SetResliceAxes(axes);
    reslice->SetResliceAxes(invAxes);
    reslice->SetInterpolationModeToLinear();
    reslice->Update();

    using VTKToITKFilterType = itk::VTKImageToImageFilter<ImageType>;
    auto vtkToItk = VTKToITKFilterType::New();
    vtkToItk->SetInput(reslice->GetOutput());
    vtkToItk->Update();

    auto output = vtkToItk->GetOutput();

    using DeepCopyFilterType = itk::ImageDuplicator<ImageType>;
    auto duplicator = DeepCopyFilterType::New();
    duplicator->SetInputImage(output);
    duplicator->Update();

    return duplicator->GetOutput();
}

ImageOrientationResampler::ImageType::Pointer
ImageOrientationResampler::ResampleToCT(ImageType::Pointer inputImage, ImageType::Pointer ctImage)
{
    using ResampleType = itk::ResampleImageFilter<ImageType, ImageType>;
    using InterpolatorType = itk::LinearInterpolateImageFunction<ImageType, double>;

    auto resample = ResampleType::New();
    resample->SetInput(inputImage);
    resample->SetOutputOrigin(ctImage->GetOrigin());
    resample->SetOutputSpacing(ctImage->GetSpacing());
    resample->SetOutputDirection(ctImage->GetDirection());
    resample->SetSize(ctImage->GetLargestPossibleRegion().GetSize());

    auto interpolator = InterpolatorType::New();
    resample->SetInterpolator(interpolator);
    resample->SetDefaultPixelValue(-1000);
    resample->Update();

    return resample->GetOutput();
}

void ImageOrientationResampler::WriteDicomSeries(ImageType::Pointer image,
    const std::vector<std::string>& outputFilenames)
{
    qDebug() << "Buffer pointer:" << image->GetBufferPointer();

    auto dicomIO = ImageIOType::New();
    auto writer = WriterType::New();
    writer->SetInput(image);
    writer->SetImageIO(dicomIO);
    writer->SetFileNames(outputFilenames);
    try
    {
        writer->Update();
    }
    catch (itk::ExceptionObject& err)
    {
        qDebug() << "ITK Exception caught during DICOM write:"
            << QString::fromStdString(err.GetDescription());
    }
}

std::vector<std::string>
ImageOrientationResampler::loadDicomSeries(const QString& folderPath)
{
    QDir dir(folderPath);
    QStringList filters;
    filters << "*.dcm" << "*.DCM";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);

    std::sort(fileList.begin(), fileList.end(),
        [](const QFileInfo& a, const QFileInfo& b) {
            auto getIndex = [](const QString& name) {
                int underscorePos = name.lastIndexOf('_');
                int dotPos = name.lastIndexOf('.');
                QString numStr = name.mid(underscorePos + 1, dotPos - underscorePos - 1);
                return numStr.toInt();
                };
            return getIndex(a.fileName()) < getIndex(b.fileName());
        });

    std::vector<std::string> dicomFiles;
    dicomFiles.reserve(fileList.size());

    for (const QFileInfo& fi : fileList)
    {
        QString fileName = fi.fileName();
        if (fileName.contains("RTPLAN", Qt::CaseInsensitive)) continue;
        if (fileName.contains("RS", Qt::CaseInsensitive)) continue;

        dicomFiles.push_back(fi.absoluteFilePath().toStdString());
    }

    return dicomFiles;
}

std::vector<std::string>
ImageOrientationResampler::GenerateOutputFileNames(
    const std::vector<std::string>& inputFiles,
    const std::string& outputDir)
{
    std::vector<std::string> outputFiles;
    outputFiles.reserve(inputFiles.size());

    for (size_t i = 0; i < inputFiles.size(); ++i)
    {
        std::ostringstream oss;
        oss << outputDir << "\\image_"
            << std::setw(4) << std::setfill('0') << i
            << ".dcm";
        outputFiles.push_back(oss.str());
    }

    return outputFiles;
}

void ImageOrientationResampler::PrintDirection(itk::Image<short, 3>::Pointer image)
{
    itk::Matrix<double, 3, 3> dir = image->GetDirection();

    qDebug() << "Image Direction Matrix:";
    for (unsigned int i = 0; i < 3; ++i)
    {
        QString row;
        for (unsigned int j = 0; j < 3; ++j)
        {
            row += QString::number(dir[i][j], 'f', 6) + " ";
        }
        qDebug().noquote() << row;
    }
}

void ImageOrientationResampler::PrintMetaDataDictionary(const itk::MetaDataDictionary& dict)
{
    for (auto it = dict.Begin(); it != dict.End(); ++it)
    {
        const std::string& key = it->first;
        itk::MetaDataObjectBase* metaObj = it->second.GetPointer();

        if (metaObj)
        {
            using MetaDataStringType = itk::MetaDataObject<std::string>;
            MetaDataStringType* strObj = dynamic_cast<MetaDataStringType*>(metaObj);
            if (strObj)
            {
                std::string value = strObj->GetMetaDataObjectValue();
                qDebug() << QString::fromStdString(key)
                    << ":"
                    << QString::fromStdString(value);
            }
            else
            {
                qDebug() << QString::fromStdString(key) << ": (non-string value)";
            }
        }
    }
}

int ImageOrientationResampler::WriteSlicesUsingNames(
    itk::Image<short, 3>::Pointer image3D,
    const std::vector<std::string>& outputDicomFiles,
    const itk::MetaDataDictionary& baseDict)
{
    // 定义类型别名
    using PixelType = short;
    constexpr unsigned int Dimension3D = 3;
    constexpr unsigned int Dimension2D = 2;
    using Image3D = itk::Image<PixelType, Dimension3D>;
    using Image2D = itk::Image<PixelType, Dimension2D>;
    using ExtractFilterType = itk::ExtractImageFilter<Image3D, Image2D>;
    using WriterType = itk::ImageFileWriter<Image2D>;

    // 检查输入图像是否为空
    if (!image3D)
    {
        std::cerr << "Input image is null." << std::endl;
        return EXIT_FAILURE;
    }

    // 获取图像的空间信息
    Image3D::SpacingType spacing = image3D->GetSpacing();     // 像素间距
    Image3D::PointType origin = image3D->GetOrigin();         // 原点坐标
    Image3D::DirectionType direction = image3D->GetDirection(); // 方向矩阵
    Image3D::RegionType region3D = image3D->GetLargestPossibleRegion();
    Image3D::SizeType size3D = region3D.GetSize();
    const unsigned int nz = size3D[2]; // Z 方向层数

    // 检查输出文件数量是否与切片数一致
    if (outputDicomFiles.size() != nz)
    {
        std::cerr << "Error: outputDicomFiles.size() (" << outputDicomFiles.size()
            << ") != number of slices (" << nz << ")." << std::endl;
        return EXIT_FAILURE;
    }

    // UID 生成器：生成一个 Series UID，每层一个 SOPInstanceUID
    gdcm::UIDGenerator uidGen;
    std::string seriesUID = uidGen.Generate();
    const std::string sopClassUID = "1.2.840.10008.5.1.4.1.1.2"; // CT Image Storage
    const std::string modality = "CT";

    auto gdcmIO = itk::GDCMImageIO::New();

    // 遍历每一层切片
    for (unsigned int k = 0; k < nz; ++k)
    {
        // 提取第 k 层区域
        Image3D::IndexType start = region3D.GetIndex();
        Image3D::SizeType size = region3D.GetSize();
        start[2] = static_cast<long>(k); // 设置起始索引为第 k 层
        size[2] = 0;                     // 只取一层
        Image3D::RegionType extractRegion;
        extractRegion.SetIndex(start);
        extractRegion.SetSize(size);

        // 使用 ExtractImageFilter 提取 2D 切片
        auto extract = ExtractFilterType::New();
        extract->SetExtractionRegion(extractRegion);
        extract->SetInput(image3D);
#if ITK_VERSION_MAJOR >= 4
        extract->SetDirectionCollapseToSubmatrix(); // 保持方向矩阵一致
#endif
        try { extract->Update(); }
        catch (itk::ExceptionObject& err) {
            std::cerr << "Extract error: " << err << std::endl;
            return EXIT_FAILURE;
        }

        Image2D::Pointer slice2D = extract->GetOutput();

        // 复制外部传入的字典（基础标签）
        itk::MetaDataDictionary dict = baseDict;

        // 计算 ImagePositionPatient (IPP)
        Image3D::PointType ipp;
        for (unsigned int r = 0; r < 3; ++r)
        {
            ipp[r] = origin[r] + direction[r][2] * spacing[2] * static_cast<double>(k);
        }
        std::ostringstream ippStream;
        ippStream << std::fixed << std::setprecision(6)
            << ipp[0] << "\\" << ipp[1] << "\\" << ipp[2];
        std::string ippString = ippStream.str();

        // 计算 ImageOrientationPatient (IOP)
        std::ostringstream iopStream;
        iopStream << std::fixed << std::setprecision(12);
        for (unsigned int c = 0; c < 2; ++c)
        {
            for (unsigned int r = 0; r < 3; ++r)
            {
                iopStream << direction[r][c];
                if (!(c == 1 && r == 2)) iopStream << "\\";
            }
        }
        std::string iopString = iopStream.str();

        // PixelSpacing 和 SliceThickness
        std::ostringstream psStream;
        psStream << spacing[0] << "\\" << spacing[1];
        std::string pixelSpacing = psStream.str();
        std::ostringstream stStream;
        stStream << spacing[2];
        std::string sliceThickness = stStream.str();

        // InstanceNumber 与 SOPInstanceUID
        std::string instanceNumber = std::to_string(k + 1);
        std::string sopInstanceUID = uidGen.Generate();

        // 填充每层特有的 DICOM 标签
        itk::EncapsulateMetaData<std::string>(dict, "0008|0060", modality);         // Modality
        itk::EncapsulateMetaData<std::string>(dict, "0020|000e", seriesUID);        // SeriesInstanceUID
        itk::EncapsulateMetaData<std::string>(dict, "0008|0016", sopClassUID);      // SOPClassUID
        itk::EncapsulateMetaData<std::string>(dict, "0008|0018", sopInstanceUID);   // SOPInstanceUID
        itk::EncapsulateMetaData<std::string>(dict, "0020|0032", ippString);        // ImagePositionPatient
        itk::EncapsulateMetaData<std::string>(dict, "0020|0037", iopString);        // ImageOrientationPatient
        itk::EncapsulateMetaData<std::string>(dict, "0020|0013", instanceNumber);   // InstanceNumber
        itk::EncapsulateMetaData<std::string>(dict, "0028|0030", pixelSpacing);     // PixelSpacing
        itk::EncapsulateMetaData<std::string>(dict, "0018|0050", sliceThickness);   // SliceThickness

        // 设置字典到切片
        slice2D->SetMetaDataDictionary(dict);

        // 写文件
        auto writer = WriterType::New();
        writer->SetInput(slice2D);
        writer->SetImageIO(gdcmIO);
        writer->SetFileName(outputDicomFiles[k]);

        try { writer->Update(); }
        catch (itk::ExceptionObject& err) {
            std::cerr << "Write error for slice " << k << " file " << outputDicomFiles[k]
                << " : " << err << std::endl;
            return EXIT_FAILURE;
        }

        // 输出日志信息
        std::cout << "Wrote: " << outputDicomFiles[k]
            << "  IPP=" << ippString
            << "  IOP=" << iopString << std::endl;
    }

    std::cout << "All slices written. SeriesInstanceUID=" << seriesUID << std::endl;
    return EXIT_SUCCESS;
}

std::string ImageOrientationResampler::GetDicomTag(const std::string& filename, const std::string& tag)
{
    auto gdcmIO = itk::GDCMImageIO::New();
    gdcmIO->SetFileName(filename);
    try {
        gdcmIO->ReadImageInformation();
    }
    catch (...) {
        return "";
    }
    itk::MetaDataDictionary dict = gdcmIO->GetMetaDataDictionary();
    std::string value;
    if (itk::ExposeMetaData<std::string>(dict, tag, value)) {
        return value;
    }
    return "";
}

int ImageOrientationResampler::ExtractTrailingNumber(const std::string& filename)
{
    std::regex re("(\\d+)(\\.dcm)$");
    std::smatch match;
    if (std::regex_search(filename, match, re)) {
        return std::stoi(match[1].str());
    }
    return -1; // 没找到数字
}

std::vector<std::string> ImageOrientationResampler::SortDicomFiles(const std::vector<std::string>& dicomFiles)
{
    std::vector<std::pair<std::string, double>> filesWithKey;

    for (auto& f : dicomFiles)
    {
        // 优先用 InstanceNumber
        std::string inst = GetDicomTag(f, "0020|0013");
        double key = -1;
        if (!inst.empty()) {
            key = std::stod(inst);
        }
        else {
            // 尝试用 ImagePositionPatient Z 坐标
            std::string ipp = GetDicomTag(f, "0020|0032");
            if (!ipp.empty()) {
                // ipp 格式 "x\y\z"
                size_t lastSlash = ipp.find_last_of("\\");
                if (lastSlash != std::string::npos) {
                    key = std::stod(ipp.substr(lastSlash + 1));
                }
            }
            else {
                // fallback: 文件名最后数字
                key = ExtractTrailingNumber(f);
            }
        }
        filesWithKey.push_back({ f,key });
    }

    std::sort(filesWithKey.begin(), filesWithKey.end(),
        [](auto& a, auto& b) { return a.second < b.second; });

    std::vector<std::string> sorted;
    for (auto& p : filesWithKey) sorted.push_back(p.first);
    return sorted;
}
