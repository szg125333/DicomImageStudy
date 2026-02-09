#include "DicomImageStudy.h"
#include <QtWidgets/QApplication>
#include "vld.h"
#include "itkImageToVTKImageFilter.h"
#include "ThreeViewWidget.h"
#include "ImageOrientationResampler.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    ImageOrientationResampler resampler;
    std::vector<std::string> dicomFiles= resampler.loadDicomSeries("C:\\Workspace\\testData\\registrationData\\Head1\\CBCT");
    dicomFiles= resampler.SortDicomFiles(dicomFiles);
    auto cbctImage = resampler.ReadDicomSeries(dicomFiles);    // ∂¡»° CBCT –Ú¡–

    using ITKToVTKFilterType = itk::ImageToVTKImageFilter<itk::Image<short, 3>>;
    auto itkToVtk = ITKToVTKFilterType::New();
    itkToVtk->SetInput(cbctImage);
    itkToVtk->Update();

    ThreeViewWidget w; 
    w.SetImageData(itkToVtk->GetOutput());
    w.resize(1200, 800);
    w.show();

    return app.exec();
}

