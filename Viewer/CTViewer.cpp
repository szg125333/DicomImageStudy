// CTViewer.cpp
#include "CTViewer.h"
#include <vtkMetaImageWriter.h>

#include <itkGDCMSeriesFileNames.h>
#include <itkGDCMImageIO.h>
#include <iostream>
#include <string>
#include <vector>
#include <itkImage.h>
#include <itkImageSeriesReader.h>
#include <itkImageToVTKImageFilter.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <itkImageFileWriter.h>
#include <itkMetaImageIOFactory.h>
#include <vtkMatrix4x4.h>
#include <vtkImageReslice.h>
#include <qdebug.h>

CTViewer::CTViewer(const std::string& dicomFolder)
    : dicomFolder(dicomFolder) {
    itk::MetaImageIOFactory::RegisterOneFactory();

}

std::vector<std::string> CTViewer::GetAllCTFilePaths(const std::string& dicomFolder)
{
    typedef itk::GDCMSeriesFileNames NamesGeneratorType;
    NamesGeneratorType::Pointer nameGenerator = NamesGeneratorType::New();
    nameGenerator->SetUseSeriesDetails(true);
    nameGenerator->SetDirectory(dicomFolder);

    // 获取所有序列 UID
    std::vector<std::string> seriesUIDs = nameGenerator->GetSeriesUIDs();
    std::vector<std::string> ctFilePaths;

    if (seriesUIDs.empty())
    {
        std::cerr << "No DICOM series found in: " << dicomFolder << std::endl;
        return ctFilePaths;
    }

    // 遍历所有序列，筛选出 Modality = CT 的序列
    itk::GDCMImageIO::Pointer dicomIO = itk::GDCMImageIO::New();

    for (auto& uid : seriesUIDs)
    {
        std::vector<std::string> fileNames = nameGenerator->GetFileNames(uid);

        if (fileNames.empty()) continue;

        // 读取第一张，检查 Modality
        dicomIO->SetFileName(fileNames[0]);
        dicomIO->ReadImageInformation();

        std::string modality;
        if (dicomIO->GetMetaDataDictionary().HasKey("0008|0060"))
        {
            itk::MetaDataDictionary& dict = dicomIO->GetMetaDataDictionary();
            itk::MetaDataObjectBase::Pointer entry = dict["0008|0060"];
            itk::MetaDataObject<std::string>* entryValue =
                dynamic_cast<itk::MetaDataObject<std::string>*>(entry.GetPointer());
            if (entryValue)
                modality = entryValue->GetMetaDataObjectValue();
        }

        if (modality == "CT")
        {
            // 把该序列的所有文件路径加入集合
            ctFilePaths.insert(ctFilePaths.end(), fileNames.begin(), fileNames.end());
        }
    }

    std::cout << "Collected " << ctFilePaths.size() << " CT files." << std::endl;
    return ctFilePaths;
}

vtkSmartPointer<vtkImageData> CTViewer::getVTKImage() 
{
    std::vector<std::string> ctFilePaths = GetAllCTFilePaths(dicomFolder);

    typedef short InputPixelType;   // CT 常用 short
    const unsigned int Dimension = 3;
    typedef itk::Image<InputPixelType, Dimension> InputImageType;
    typedef itk::ImageSeriesReader<InputImageType> ReaderType;

    // 1. 创建 reader
    itk::GDCMImageIO::Pointer dicomIO = itk::GDCMImageIO::New();
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetImageIO(dicomIO);
    reader->SetFileNames(ctFilePaths);
    reader->ReleaseDataFlagOn();

    try
    {
        reader->Update();   // 真正读入所有 CT 文件
    }
    catch (itk::ExceptionObject& ex)
    {
        qDebug() << "ITK Reader Exception: " << ex.what();
        return nullptr;
    }

    InputImageType::Pointer image = reader->GetOutput();

    typedef itk::ImageFileWriter<InputImageType> WriterType;
    WriterType::Pointer writer = WriterType::New();
    writer->SetFileName("output.mha");
    writer->SetInput(image);

    try
    {
        writer->Update();
    }
    catch (itk::ExceptionObject& ex)
    {
        qDebug() << "Writer Exception: " << ex.what();
    }


    // 2. ITK → VTK 转换
    //typedef itk::ImageToVTKImageFilter<InputImageType> ConnectorType;
    connector = ConnectorType::New();
    connector->SetInput(image);

    try
    {
        connector->Update();
    }
    catch (itk::ExceptionObject& ex)
    {
        std::cerr << "Connector Exception: " << ex << std::endl;
        return nullptr;
    }

    //// 3. 返回 vtkImageData*
    //vtkSmartPointer<vtkImageData> vtkImage = connector->GetOutput();
    // 1. 获取 ITK 方向矩阵（LPS）
    InputImageType::DirectionType itkDir = image->GetDirection();
    // 保存方向矩阵
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            direction[i * 3 + j] = itkDir[i][j];
        }
    }
    InputImageType::PointType itkOrigin = image->GetOrigin();
    origin[0] = itkOrigin[0];
    origin[1] = itkOrigin[1];
    origin[2] = itkOrigin[2];

    // 2. 构造 RAS 方向矩阵
    vtkNew<vtkMatrix4x4> rasMatrix;
    rasMatrix->Identity();
    qDebug() << "RAS Matrix:";
    for (int i = 0; i < 3; i++)
    {
        QString row;
        for (int j = 0; j < 3; j++)
        {
            row += QString::number(rasMatrix->GetElement(i, j)) + " ";
            double v = itkDir[i][j];
            rasMatrix->SetElement(i, j, v);
        }
        qDebug().noquote() << row;
    }

    // 3. 用 vtkImageReslice 应用方向矩阵
    vtkNew<vtkImageReslice> reslice;
    reslice->SetInputData(connector->GetOutput());
    reslice->SetResliceAxes(rasMatrix);
    reslice->SetInterpolationModeToLinear();
    reslice->Update();

    // 4. 得到方向正确的 VTK 图像
    vtkSmartPointer<vtkImageData> vtkImage = reslice->GetOutput();


    // ========== 2. 保存为 MetaImage 格式（.mhd + .raw） ==========
    vtkSmartPointer<vtkMetaImageWriter> writer1= vtkSmartPointer<vtkMetaImageWriter>::New();
    writer1->SetInputData(vtkImage);
    writer1->SetFileName("CT_Saved1.mhd"); // .raw 文件会自动生成在同目录
    // 二进制模式保存（体积更小）
    writer1->SetCompression(false); // 若需要压缩可设为 true
    writer1->Write();


    //vtkImage->DeepCopy(connector->GetOutput());
    return vtkImage;
}

