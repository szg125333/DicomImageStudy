#include "DicomImageStudy.h"
#include "ImageOrientationResampler.h"
#include <QtWidgets/QApplication>
#include "itkImageFileWriter.h"
#include "itkMetaImageIO.h"
#include "itkImageDuplicator.h"
#include "vld.h"
#include "itkImage.h" 
#include "itkImageSeriesWriter.h" 
#include "itkGDCMImageIO.h"
#include "itkNumericSeriesFileNames.h"
#include "itkMetaDataDictionary.h" 
#include "itkMetaDataObject.h"
#include "itkImageToVTKImageFilter.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

#include "itkImageFileReader.h"
#include "itkExtractImageFilter.h"
#include <gdcmUIDGenerator.h>
#include <QGridLayout.h>
#include "ThreeViewWidget.h"


// 故意制造内存泄漏的函数
void LeakSomeMemory() {
    // 1. 在堆上分配内存
    int* pLeak = new int(42); // 分配了 4 个字节，但没有保存指针
}

// 另一个常见的泄漏：忘记释放数组
void LeakArray() {
    char* pBuffer = new char[100];
    // 忘记调用 delete[] pBuffer;
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    //// 调用泄漏函数
    //LeakSomeMemory();
    //LeakArray();
    //using ImageType = itk::Image<short, 3>; using SliceType = itk::Image<short, 2>;
    //DicomImageStudy window;
    //window.resize(800, 600);

    //window.loadDicom("C:\\Workspace\\testData\\registrationData\\Head1\\CT");
    //window.loadDicom("C:\\Workspace\\testData\\HFP");
    //window.show();

    ImageOrientationResampler resampler;
    ////std::vector<std::string> dicomFiles= resampler.loadDicomSeries("C:\\Workspace\\testData\\registrationData\\Head1\\CBCT");
    std::vector<std::string> dicomFiles= resampler.loadDicomSeries("C:\\Workspace\\testData\\HFP");
    dicomFiles= resampler.SortDicomFiles(dicomFiles);
    auto cbctImage = resampler.ReadDicomSeries(dicomFiles);    // 读取 CBCT 序列
    //itk::MetaDataDictionary baseDict = cbctImage->GetMetaDataDictionary();
    //// 从原始图像获取字典
    //// 修改体位相关标签
    //itk::EncapsulateMetaData<std::string>(baseDict, "0018|5100", "HFS"); // PatientPosition
    //// 重新生成 Study/Series UID
    //gdcm::UIDGenerator uidGen;
    //std::string newStudyUID = uidGen.Generate();
    //std::string newFrameUID = uidGen.Generate();
    //itk::EncapsulateMetaData<std::string>(baseDict, "0020|000D", newStudyUID);
    //itk::EncapsulateMetaData<std::string>(baseDict, "0020|0052", newFrameUID);

    //std::string iopString = "1\\0\\0\\0\\1\\0";

    //// 写入字典
    //itk::EncapsulateMetaData<std::string>(baseDict, "0020|0037", iopString);

    //// 其他患者信息可以保持不变或按需要修改
    //// itk::EncapsulateMetaData<std::string>(baseDict, "0010|0010", "PatientName");
    //// itk::EncapsulateMetaData<std::string>(baseDict, "0010|0020", "PatientID");

    //// 写出新序列

    //// 体位变换 (HFP → HFS)
    //std::array<double, 6> orientation = { -1,0,0, 0,-1,0 }; // HFP
    //itk::Image<short, 3>::Pointer cbctHfsImage = resampler.ApplyOrientationTransform(cbctImage, orientation);
    //std::vector<std::string> outputDicomFiles = resampler.GenerateOutputFileNames(dicomFiles, "C:\\Workspace\\testData\\PositionTest\\HFP");
    ////resampler.WriteSlicesUsingNames(cbctHfsImage, outputDicomFiles);
    //resampler.WriteSlicesUsingNames(cbctHfsImage, outputDicomFiles, baseDict);



    using ITKToVTKFilterType = itk::ImageToVTKImageFilter<itk::Image<short, 3>>;
    auto itkToVtk = ITKToVTKFilterType::New();
    itkToVtk->SetInput(cbctImage);
    itkToVtk->Update();

    ThreeViewWidget w; 
    w.SetImageData(itkToVtk->GetOutput());
    w.resize(1200, 400);
    w.show();

    return app.exec();
}

