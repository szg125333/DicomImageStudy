#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include "vld.h"
#include "itkImageToVTKImageFilter.h"

#include "UI/DicomImageStudy.h"
#include "UI/ThreeViewWidget.h"
#include "Utils/ImageOrientationResampler.h"
#include "UI/StartWidget.h"
#include "UI/TitleBarWidget.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QString exeDir = QCoreApplication::applicationDirPath();
    QString logPath = exeDir + "/memory_leak_report.txt";
    std::wstring wlog = logPath.toStdWString();
    VLDSetReportOptions(VLD_OPT_REPORT_TO_FILE, wlog.c_str());

    ImageOrientationResampler resampler;
    std::vector<std::string> dicomFiles= resampler.loadDicomSeries("C:\\Workspace\\testData\\registrationData\\Head1\\CBCT");
    dicomFiles= resampler.SortDicomFiles(dicomFiles);
    auto cbctImage = resampler.ReadDicomSeries(dicomFiles);    // 读取 CBCT 序列

    using ITKToVTKFilterType = itk::ImageToVTKImageFilter<itk::Image<short, 3>>;
    auto itkToVtk = ITKToVTKFilterType::New();
    itkToVtk->SetInput(cbctImage);
    itkToVtk->Update();

    StartWidget startWidget;

    ThreeViewWidget* threeViewWidget = new ThreeViewWidget(&startWidget);
    TitleBarWidget* titleBarWidget = new TitleBarWidget(&startWidget);

    threeViewWidget->SetImageData(itkToVtk->GetOutput());

    // 关键：让影像区自动扩展
    threeViewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(startWidget.layout());
    if (mainLayout)
    {
        // 创建一个容器，把头部栏和影像区放在一起
        QWidget* contentWidget = new QWidget(&startWidget);
        QVBoxLayout* vBoxLayout = new QVBoxLayout(contentWidget);
        vBoxLayout->setContentsMargins(10, 10, 10, 10);

        // 上面是头部栏，下面是影像区
        vBoxLayout->addWidget(titleBarWidget);
        vBoxLayout->addWidget(threeViewWidget, /*stretch*/ 1);

        // 插入到工具栏和状态栏之间
        mainLayout->insertWidget(2, contentWidget, 1);
    }

    startWidget.resize(1400, 900);
    startWidget.show();


    QObject::connect(titleBarWidget,&TitleBarWidget::requestEnableDistanceMeasurement, threeViewWidget,&ThreeViewWidget::setModeToDistanceMeasurement);

    return app.exec();
}

