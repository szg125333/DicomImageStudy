#include "DicomLoader.h"

#include <itkGDCMSeriesFileNames.h>
#include <itkGDCMImageIO.h>
#include <itkImageSeriesReader.h>
#include <itkImageToVTKImageFilter.h>
#include <itkResampleImageFilter.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkIdentityTransform.h>
#include <itkAffineTransform.h>
#include <itkMetaDataObject.h>

#include <vtkImageData.h>
#include <vtkImageReslice.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>

#include <QDir>
#include <QDebug>

#include <algorithm>
#include <iostream>
#include <regex>

// ============================================================
//  公共接口
// ============================================================

vtkSmartPointer<vtkImageData> DicomLoader::Load(const std::string& dicomFolder) {
    auto files = loadSortedDicomFiles(dicomFolder);
    if (files.empty()) return nullptr;

    auto itkImage = readDicomAsITK(files);
    if (!itkImage) return nullptr;

    return itkToVtk(itkImage);
}

vtkSmartPointer<vtkImageData> DicomLoader::LoadAndResample(
    const std::string& cbctFolder,
    const std::string& referenceCtFolder)
{
    // 读取参考 CT
    auto ctFiles = loadSortedDicomFiles(referenceCtFolder);
    if (ctFiles.empty()) {
        std::cerr << "[DicomLoader] Reference CT folder is empty: " << referenceCtFolder << std::endl;
        return nullptr;
    }
    auto ctImage = readDicomAsITK(ctFiles);
    if (!ctImage) return nullptr;

    return LoadAndResample(cbctFolder, ctImage);
}

vtkSmartPointer<vtkImageData> DicomLoader::LoadAndResample(
    const std::string& cbctFolder,
    ITKImageType::Pointer referenceCtImage)
{
    // 读取 CBCT
    auto cbctFiles = loadSortedDicomFiles(cbctFolder);
    if (cbctFiles.empty()) {
        std::cerr << "[DicomLoader] CBCT folder is empty: " << cbctFolder << std::endl;
        return nullptr;
    }
    auto cbctImage = readDicomAsITK(cbctFiles);
    if (!cbctImage) return nullptr;

    std::cout << "[DicomLoader] CBCT loaded, resampling to CT grid..." << std::endl;

    // 打印重采样前后的信息
    auto cbctSize = cbctImage->GetLargestPossibleRegion().GetSize();
    auto ctSize = referenceCtImage->GetLargestPossibleRegion().GetSize();
    std::cout << "[DicomLoader] CBCT size: " << cbctSize[0] << "x" << cbctSize[1] << "x" << cbctSize[2]
        << " spacing: " << cbctImage->GetSpacing()[0] << "," << cbctImage->GetSpacing()[1] << "," << cbctImage->GetSpacing()[2]
        << std::endl;
    std::cout << "[DicomLoader] CT   size: " << ctSize[0] << "x" << ctSize[1] << "x" << ctSize[2]
        << " spacing: " << referenceCtImage->GetSpacing()[0] << "," << referenceCtImage->GetSpacing()[1] << "," << referenceCtImage->GetSpacing()[2]
        << std::endl;

    // 重采样
    auto resampled = resampleToReference(cbctImage, referenceCtImage);
    if (!resampled) return nullptr;

    auto resSize = resampled->GetLargestPossibleRegion().GetSize();
    std::cout << "[DicomLoader] Resampled size: " << resSize[0] << "x" << resSize[1] << "x" << resSize[2]
        << " (should match CT)" << std::endl;

    return itkToVtk(resampled);
}

// ============================================================
//  私有方法
// ============================================================

std::vector<std::string> DicomLoader::loadSortedDicomFiles(const std::string& folder) {
    using NamesGeneratorType = itk::GDCMSeriesFileNames;
    auto nameGenerator = NamesGeneratorType::New();
    nameGenerator->SetUseSeriesDetails(true);
    nameGenerator->SetDirectory(folder);

    auto seriesUIDs = nameGenerator->GetSeriesUIDs();
    if (seriesUIDs.empty()) {
        std::cerr << "[DicomLoader] No DICOM series found in: " << folder << std::endl;
        return {};
    }

    // 找最大文件数的序列（通常就是主图像序列）
    std::vector<std::string> bestFiles;
    std::string bestModality;

    auto dicomIO = itk::GDCMImageIO::New();

    for (auto& uid : seriesUIDs) {
        auto fileNames = nameGenerator->GetFileNames(uid);
        if (fileNames.empty()) continue;

        // 读取 Modality
        dicomIO->SetFileName(fileNames[0]);
        dicomIO->ReadImageInformation();

        std::string modality;
        auto& dict = dicomIO->GetMetaDataDictionary();
        if (dict.HasKey("0008|0060")) {
            auto* entry = dynamic_cast<itk::MetaDataObject<std::string>*>(
                dict["0008|0060"].GetPointer());
            if (entry) modality = entry->GetMetaDataObjectValue();
        }

        // 跳过 RT Structure、RT Plan 等非图像类型
        if (modality == "RTSTRUCT" || modality == "RTPLAN" ||
            modality == "RTDOSE" || modality == "REG") {
            continue;
        }

        // 选文件数最多的序列
        if (fileNames.size() > bestFiles.size()) {
            bestFiles = fileNames;
            bestModality = modality;
        }
    }

    std::cout << "[DicomLoader] Found " << bestFiles.size()
        << " files, modality: " << bestModality << std::endl;

    return bestFiles;
}

DicomLoader::ITKImageType::Pointer DicomLoader::readDicomAsITK(
    const std::vector<std::string>& files)
{
    if (files.empty()) return nullptr;

    auto dicomIO = itk::GDCMImageIO::New();
    auto reader = itk::ImageSeriesReader<ITKImageType>::New();
    reader->SetImageIO(dicomIO);
    reader->SetFileNames(files);

    try {
        reader->Update();
    }
    catch (itk::ExceptionObject& ex) {
        std::cerr << "[DicomLoader] ITK read error: " << ex.what() << std::endl;
        return nullptr;
    }

    return reader->GetOutput();
}

DicomLoader::ITKImageType::Pointer DicomLoader::resampleToReference(
    ITKImageType::Pointer inputImage,
    ITKImageType::Pointer referenceImage)
{
    if (!inputImage || !referenceImage) return nullptr;

    using AffineTransformType = itk::AffineTransform<double, 3>;
    using InterpolatorType = itk::LinearInterpolateImageFunction<ITKImageType, double>;
    using ResampleFilterType = itk::ResampleImageFilter<ITKImageType, ITKImageType>;

    // ============================================================
    //  计算两个图像的几何中心
    //  center = origin + (size - 1) * spacing / 2.0
    // ============================================================
    auto inputOrigin = inputImage->GetOrigin();
    auto inputSpacing = inputImage->GetSpacing();
    auto inputSize = inputImage->GetLargestPossibleRegion().GetSize();
    auto inputDirection = inputImage->GetDirection();

    auto refOrigin = referenceImage->GetOrigin();
    auto refSpacing = referenceImage->GetSpacing();
    auto refSize = referenceImage->GetLargestPossibleRegion().GetSize();
    auto refDirection = referenceImage->GetDirection();

    ITKImageType::PointType inputCenter, refCenter;
    for (int i = 0; i < 3; ++i) {
        // 考虑 direction 的情况下计算中心
        inputCenter[i] = inputOrigin[i];
        refCenter[i] = refOrigin[i];
        for (int j = 0; j < 3; ++j) {
            inputCenter[i] += inputDirection[i][j] * (inputSize[j] - 1) * inputSpacing[j] / 2.0;
            refCenter[i] += refDirection[i][j] * (refSize[j] - 1) * refSpacing[j] / 2.0;
        }
    }

    std::cout << "[DicomLoader] CBCT center: ("
        << inputCenter[0] << ", " << inputCenter[1] << ", " << inputCenter[2] << ")" << std::endl;
    std::cout << "[DicomLoader] CT   center: ("
        << refCenter[0] << ", " << refCenter[1] << ", " << refCenter[2] << ")" << std::endl;

    // ============================================================
    //  构建变换：将 CBCT 的中心平移到 CT 的中心
    //
    //  ResampleImageFilter 的 Transform 含义：
    //  对于输出空间中的每个点 p_out，变换后得到 p_in = T(p_out)，
    //  然后从输入图像中采样 p_in 处的值。
    //  所以要让 CBCT 的中心对齐到 CT 的中心，
    //  平移量 = CBCT中心 - CT中心（从输出空间映射到输入空间）
    // ============================================================
    auto transform = AffineTransformType::New();
    transform->SetIdentity();

    // 如果方向矩阵不同，还需要旋转对齐
    // 先处理方向差异：R = inputDirection * refDirection^(-1)
    auto refDirInverse = ITKImageType::DirectionType(refDirection.GetInverse());
    auto rotationMatrix = AffineTransformType::MatrixType();
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            double val = 0;
            for (int k = 0; k < 3; ++k)
                val += inputDirection[i][k] * refDirInverse[k][j];
            rotationMatrix[i][j] = val;
        }

    transform->SetMatrix(rotationMatrix);

    // 平移：先旋转 CT 中心到输入空间，再算偏移
    AffineTransformType::OutputVectorType offset;
    for (int i = 0; i < 3; ++i) {
        offset[i] = inputCenter[i] - refCenter[i];
    }

    // SetCenter + SetTranslation 的组合：
    // T(x) = R * (x - center) + center + translation
    // 等价于绕 refCenter 旋转，然后平移 offset
    transform->SetCenter(refCenter);
    AffineTransformType::OutputVectorType translation;
    for (int i = 0; i < 3; ++i) {
        double rotatedRefCenter = 0;
        for (int j = 0; j < 3; ++j)
            rotatedRefCenter += rotationMatrix[i][j] * refCenter[j];
        translation[i] = inputCenter[i] - rotatedRefCenter;
    }
    transform->SetTranslation(translation);

    std::cout << "[DicomLoader] Transform translation: ("
        << translation[0] << ", " << translation[1] << ", " << translation[2] << ")" << std::endl;

    // ============================================================
    //  重采样
    // ============================================================
    auto interpolator = InterpolatorType::New();
    auto resampler = ResampleFilterType::New();

    resampler->SetInput(inputImage);
    resampler->SetTransform(transform);
    resampler->SetInterpolator(interpolator);

    // 目标网格参数全部取自 CT
    resampler->SetOutputOrigin(referenceImage->GetOrigin());
    resampler->SetOutputSpacing(referenceImage->GetSpacing());
    resampler->SetOutputDirection(referenceImage->GetDirection());
    resampler->SetSize(referenceImage->GetLargestPossibleRegion().GetSize());

    // 超出范围填充空气 HU 值
    resampler->SetDefaultPixelValue(-1024);

    try {
        resampler->Update();
    }
    catch (itk::ExceptionObject& ex) {
        std::cerr << "[DicomLoader] Resample error: " << ex.what() << std::endl;
        return nullptr;
    }

    std::cout << "[DicomLoader] Resample complete" << std::endl;

    return resampler->GetOutput();
}

vtkSmartPointer<vtkImageData> DicomLoader::itkToVtk(ITKImageType::Pointer itkImage) {
    if (!itkImage) return nullptr;

    // ITK → VTK
    using ConnectorType = itk::ImageToVTKImageFilter<ITKImageType>;
    auto connector = ConnectorType::New();
    connector->SetInput(itkImage);

    try {
        connector->Update();
    }
    catch (itk::ExceptionObject& ex) {
        std::cerr << "[DicomLoader] ITK→VTK error: " << ex.what() << std::endl;
        return nullptr;
    }

    // 应用方向矩阵（ITK 的 Direction → VTK 的 Reslice）
    auto direction = itkImage->GetDirection();

    vtkNew<vtkMatrix4x4> rasMatrix;
    rasMatrix->Identity();
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            rasMatrix->SetElement(i, j, direction[i][j]);

    vtkNew<vtkImageReslice> reslice;
    reslice->SetInputData(connector->GetOutput());
    reslice->SetResliceAxes(rasMatrix);
    reslice->SetInterpolationModeToLinear();
    reslice->Update();

    // 深拷贝（reslice 的输出生命周期不可靠）
    auto result = vtkSmartPointer<vtkImageData>::New();
    result->DeepCopy(reslice->GetOutput());

    return result;
}