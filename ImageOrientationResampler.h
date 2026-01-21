#include "itkImage.h"
#include "itkImageSeriesReader.h"
#include "itkImageSeriesWriter.h"
#include "itkGDCMImageIO.h"
#include "itkResampleImageFilter.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkEuler3DTransform.h"
#include "itkImageToVTKImageFilter.h"
#include "itkVTKImageToImageFilter.h"
#include "itkImageFileWriter.h"
#include "itkMetaImageIO.h"
#include "itkImageDuplicator.h"

#include <vtkImageReslice.h>
#include <vtkMatrix4x4.h>
#include <vtkSmartPointer.h>
#include <vtkMath.h>
#include <vtkMetaImageWriter.h>

#include <QDir> 
#include <QFileInfoList> 
#include <QStringList> 
#include <QDebug> 
#include <iostream>


class ImageOrientationResampler
{
public:
    using PixelType = short;
    using ImageType = itk::Image<PixelType, 3>;
    using ReaderType = itk::ImageSeriesReader<ImageType>;
    using WriterType = itk::ImageSeriesWriter<ImageType, itk::Image<short, 2>>;
    using ImageIOType = itk::GDCMImageIO;

    ImageOrientationResampler() {}

    // 读取 DICOM 序列
    ImageType::Pointer ReadDicomSeries(const std::vector<std::string>& filenames)
    {
        auto reader = ReaderType::New();
        auto dicomIO = ImageIOType::New();
        reader->SetImageIO(dicomIO);
        reader->SetFileNames(filenames);
        reader->Update();
        return reader->GetOutput();
    }

    // 根据行列式 6 个值进行体位变换 (ITK→VTK→Reslice→ITK)
    ImageType::Pointer ApplyOrientationTransform(ImageType::Pointer inputImage,
        const std::array<double, 6>& orientation)
    {
        // ITK → VTK
        using ITKToVTKFilterType = itk::ImageToVTKImageFilter<ImageType>;
        auto itkToVtk = ITKToVTKFilterType::New();
        itkToVtk->SetInput(inputImage);
        itkToVtk->Update();

        vtkSmartPointer<vtkImageData> vtkImg = vtkSmartPointer<vtkImageData>::New();
        vtkImg->DeepCopy(itkToVtk->GetOutput());

        // 构造方向矩阵
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

        //vtkSmartPointer<vtkMatrix4x4> invAxes = vtkSmartPointer<vtkMatrix4x4>::New();
        //invAxes->DeepCopy(axes);
        //invAxes->Invert();

        // VTK Reslice
        vtkNew<vtkImageReslice> reslice;
        reslice->SetInputData(vtkImg);
        reslice->SetResliceAxes(axes);
        //reslice->SetResliceAxes(invAxes);
        reslice->SetInterpolationModeToLinear();
        reslice->Update();

        // VTK → ITK
        using VTKToITKFilterType = itk::VTKImageToImageFilter<ImageType>;
        auto vtkToItk = VTKToITKFilterType::New();
        vtkToItk->SetInput(reslice->GetOutput());
        vtkToItk->Update();

        auto output = vtkToItk->GetOutput();

        // 👇 关键：深拷贝图像，使其脱离 filter 生命周期
        using DeepCopyFilterType = itk::ImageDuplicator<ImageType>;
        auto duplicator = DeepCopyFilterType::New();
        duplicator->SetInputImage(output);
        duplicator->Update();

        return duplicator->GetOutput(); // 返回独立副本
    }

    // 重采样到目标 CT 几何
    ImageType::Pointer ResampleToCT(ImageType::Pointer inputImage, ImageType::Pointer ctImage)
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

    // 保存 DICOM 序列
    void WriteDicomSeries(ImageType::Pointer image,
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

    std::vector<std::string> loadDicomSeries(const QString& folderPath)
    {
        QDir dir(folderPath);
        QStringList filters;
        filters << "*.dcm" << "*.DCM";   // 只读取 DICOM 文件
        QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);

        // 按文件名中的数字排序
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
        dicomFiles.reserve(fileList.size()); // 预分配空间

        for (const QFileInfo& fi : fileList)
        {
            QString fileName = fi.fileName();

            // 排除 RTPLAN 和 RS 文件
            if (fileName.contains("RTPLAN", Qt::CaseInsensitive)) continue;
            if (fileName.contains("RS", Qt::CaseInsensitive)) continue;

            dicomFiles.push_back(fi.absoluteFilePath().toStdString());
        }

        return dicomFiles;
    }


    // 根据输入文件列表和输出目录生成新的输出文件名列表
    std::vector<std::string> GenerateOutputFileNames(
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

    void PrintDirection(itk::Image<short, 3>::Pointer image)
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

    void PrintMetaDataDictionary(const itk::MetaDataDictionary& dict)
    {
        for (auto it = dict.Begin(); it != dict.End(); ++it)
        {
            const std::string& key = it->first;
            itk::MetaDataObjectBase* metaObj = it->second.GetPointer();

            if (metaObj)
            {
                // 尝试转换成字符串类型
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
};
