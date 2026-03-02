#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QSplitter>
#include <QTimer>
#include "vld.h"
#include "itkImageToVTKImageFilter.h"

#include "UI/DicomImageStudy.h"
#include "UI/ThreeViewWidget.h"
#include "Utils/ImageOrientationResampler.h"
#include "UI/StartWidget.h"
#include "UI/TitleBarWidget.h"
#include "UI/LeftToolWidget.h"
#include "Dicom/DicomMetadataExtractor.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QString exeDir = QCoreApplication::applicationDirPath();
    QString logPath = exeDir + "/memory_leak_report.txt";
    std::wstring wlog = logPath.toStdWString();
    VLDSetReportOptions(VLD_OPT_REPORT_TO_FILE, wlog.c_str());

    QString path = "C:\\Workspace\\testData\\registrationData\\Head1\\CBCT";

    ImageOrientationResampler resampler;
    std::vector<std::string> dicomFiles= resampler.loadDicomSeries(path);
    dicomFiles= resampler.SortDicomFiles(dicomFiles);
    auto cbctImage = resampler.ReadDicomSeries(dicomFiles);    // 读取 CBCT 序列

    using ITKToVTKFilterType = itk::ImageToVTKImageFilter<itk::Image<short, 3>>;
    auto itkToVtk = ITKToVTKFilterType::New();
    itkToVtk->SetInput(cbctImage);
    itkToVtk->Update();

    StartWidget startWidget;

    // 创建子部件
    ThreeViewWidget* threeViewWidget = new ThreeViewWidget(&startWidget);
    TitleBarWidget* titleBarWidget = new TitleBarWidget(&startWidget);
    LeftToolWidget* leftToolWidget = new LeftToolWidget(&startWidget);

    threeViewWidget->SetImageData(itkToVtk->GetOutput());

    // === 使用 QSplitter 实现可拖拽分栏 ===
    QSplitter* centralSplitter = new QSplitter(Qt::Horizontal, &startWidget);
    centralSplitter->setHandleWidth(6);
    centralSplitter->setChildrenCollapsible(false);
    centralSplitter->addWidget(leftToolWidget);
    centralSplitter->addWidget(threeViewWidget);

    // 延迟设置 splitter 尺寸（解决 setSizes 无效问题）
    QTimer::singleShot(0, [centralSplitter]() {
        centralSplitter->setSizes({ 200, 800 });
        });

    // === 构建带弹簧的标题栏 ===
    QWidget* titleContainer = new QWidget(&startWidget);
    QHBoxLayout* titleLayout = new QHBoxLayout(titleContainer);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(0);
    titleLayout->addWidget(titleBarWidget);
    titleLayout->addStretch(1); // ← 弹簧！让标题靠左

    // === 整体垂直布局：标题栏 + 分栏区域 ===
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(startWidget.layout());
    if (mainLayout)
    {
        QWidget* contentContainer = new QWidget(&startWidget);
        QVBoxLayout* contentLayout = new QVBoxLayout(contentContainer);
        contentLayout->setContentsMargins(0, 0, 0, 0);
        contentLayout->setSpacing(0);

        contentLayout->addWidget(titleContainer, 4);      // ← 新的标题容器
        contentLayout->addWidget(centralSplitter, 96);

        mainLayout->insertWidget(2, contentContainer, 1);
    }

    startWidget.showMaximized();


    QObject::connect(titleBarWidget,&TitleBarWidget::requestEnableDistanceMeasurement, threeViewWidget,&ThreeViewWidget::setModeToDistanceMeasurement);

    QMap<QString, QString> metadata = DicomMetadataExtractor::extractFromDirectory(path);
    leftToolWidget->SetDicomMetadata(metadata);

    return app.exec();
}

