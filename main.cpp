#include "DicomImageStudy.h"
#include "ImageOrientationResampler.h"
#include <QtWidgets/QApplication>
#include "itkImageFileWriter.h"
#include "itkMetaImageIO.h"
#include "itkImageDuplicator.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);


    DicomImageStudy window;
    window.resize(800, 600);

    ////window.loadDicom("C:\\Workspace\\testData\\registrationData\\Head1\\CT");
    //window.loadDicom("C:\\Workspace\\testData\\HFP");
    //window.show();

    ImageOrientationResampler resampler;
    std::vector<std::string> dicomFiles= resampler.loadDicomSeries("C:\\Workspace\\testData\\registrationData\\Head1\\CBCT");

    auto cbctImage = resampler.ReadDicomSeries(dicomFiles);    // 读取 CBCT 序列
   
    //// 保存为 MHA
    //using WriterType = itk::ImageFileWriter<itk::Image<short, 3>>;
    //auto writer = WriterType::New();
    //writer->SetImageIO(itk::MetaImageIO::New());
    //writer->SetInput(cbctImage);
    //writer->SetFileName("test_output.mha");
    //writer->Update();

    // 体位变换 (HFP → HFS)
    //std::array<double, 6> orientation = { -1,0,0, 0,-1,0 }; // HFP
    //std::array<double, 6> orientation = { 0,1,0, -1,0,0 }; // HFDR
    std::array<double, 6> orientation = { 0,-1,0, 1,0,0 }; // HFDR
    itk::Image<short, 3>::Pointer cbctHfsImage = resampler.ApplyOrientationTransform(cbctImage, orientation);

    // 保存为 MHA
    using WriterType = itk::ImageFileWriter<itk::Image<short, 3>>;
    auto writer = WriterType::New();
    writer->SetImageIO(itk::MetaImageIO::New());
    writer->SetInput(cbctHfsImage);
    writer->SetFileName("test_output.mha");
    writer->Update();

    resampler.PrintDirection(cbctHfsImage);

    //resampler.PrintMetaDataDictionary(cbctImage->GetMetaDataDictionary());
    //// 拷贝原始 DICOM 标签
    //cbctHfsImage->SetMetaDataDictionary(cbctImage->GetMetaDataDictionary());

    //// 然后更新关键几何标签
    //auto& dict = cbctHfsImage->GetMetaDataDictionary();
    //itk::EncapsulateMetaData<std::string>(dict, "0020|0032", "HFP");                  // ImagePositionPatient
    //itk::EncapsulateMetaData<std::string>(dict, "0020|0037", "-1\\0\\0\\0\\-1\\0"); // ImageOrientationPatient
    //// 保存结果
    //std::vector<std::string> outputDicomFiles = resampler.GenerateOutputFileNames(dicomFiles, "C:\\Workspace\\testData\\PositionTest\\HFP");
    //resampler.WriteDicomSeries(cbctHfsImage, outputDicomFiles);


    return app.exec();
}
