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
#include "Utils/DicomLoader.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // ===== 内存泄漏检测 =====
    QString exeDir = QCoreApplication::applicationDirPath();
    QString logPath = exeDir + "/memory_leak_report.txt";
    std::wstring wlog = logPath.toStdWString();
    VLDSetReportOptions(VLD_OPT_REPORT_TO_FILE, wlog.c_str());

    //// ===== 数据路径 =====
    //QString ctPath = "Chest1\\CT";
    //QString cbctPath = "Chest1\\CBCT";
    //std::string rsPath = "Chest1\\CT\\RS1.2.752.243.1.1.20240509084617335.3000.36570.dcm";
    
    QString ctPath = "C:\\Workspace\\testData\\registrationData\\Head1\\CT";
    QString cbctPath = "C:\\Workspace\\testData\\registrationData\\Head1\\CBCT";
    //std::string rsPath = "Chest1\\CT\\RS1.2.752.243.1.1.20240509084617335.3000.36570.dcm";


    // ===== 数据加载 =====
    DicomLoader loader;
    auto ctImage = loader.Load(ctPath.toStdString());
    auto cbctImage = loader.LoadAndResample(cbctPath.toStdString(), ctPath.toStdString());

    // ===== UI 创建 =====
    StartWidget startWidget;
    ThreeViewWidget* threeViewWidget = new ThreeViewWidget(&startWidget);
    TitleBarWidget* titleBarWidget = new TitleBarWidget(&startWidget);
    LeftToolWidget* leftToolWidget = new LeftToolWidget(&startWidget);

    // ===== 数据注入 =====
    threeViewWidget->SetImageData(ctImage);         // CT 作为主图像
    threeViewWidget->SetOverlayImage(cbctImage);       // CBCT 作为叠加图像
    //threeViewWidget->LoadRtStruct(rsPath);
    // ===== 布局 =====
    QSplitter* centralSplitter = new QSplitter(Qt::Horizontal, &startWidget);
    centralSplitter->setHandleWidth(6);
    centralSplitter->setChildrenCollapsible(false);
    centralSplitter->addWidget(leftToolWidget);
    centralSplitter->addWidget(threeViewWidget);

    QTimer::singleShot(0, [centralSplitter]() {
        centralSplitter->setSizes({ 200, 800 });
        });

    QWidget* titleContainer = new QWidget(&startWidget);
    QHBoxLayout* titleLayout = new QHBoxLayout(titleContainer);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(0);
    titleLayout->addWidget(titleBarWidget);
    titleLayout->addStretch(1);

    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(startWidget.layout());
    if (mainLayout) {
        QWidget* contentContainer = new QWidget(&startWidget);
        QVBoxLayout* contentLayout = new QVBoxLayout(contentContainer);
        contentLayout->setContentsMargins(0, 0, 0, 0);
        contentLayout->setSpacing(0);
        contentLayout->addWidget(titleContainer, 4);
        contentLayout->addWidget(centralSplitter, 96);
        mainLayout->insertWidget(2, contentContainer, 1);
    }

    startWidget.showMaximized();

    // ===== 信号连接 =====

    // 工具栏模式切换 → 三视图（一行搞定所有模式）
    QObject::connect(titleBarWidget, &TitleBarWidget::requestMode,
        threeViewWidget, &ThreeViewWidget::setModeToMeasurement);

    // 重置视图
    QObject::connect(titleBarWidget, &TitleBarWidget::requestResetViews,
        threeViewWidget, &ThreeViewWidget::ResetAllViews);

    // 图像拖拽 → 左侧面板
    QObject::connect(threeViewWidget, &ThreeViewWidget::imageDragUpdated,
        leftToolWidget, &LeftToolWidget::OnDragUpdated);
    QObject::connect(threeViewWidget, &ThreeViewWidget::imageDragReset,
        leftToolWidget, &LeftToolWidget::OnDragReset);

    // DICOM 元数据 → 左侧面板
    QMap<QString, QString> metadata = DicomMetadataExtractor::extractFromDirectory(ctPath);
    leftToolWidget->SetDicomMetadata(metadata);

    // 轮廓列表：注入数据提供者 + 连接可见性变化
    titleBarWidget->SetContourListProvider([threeViewWidget]() {
        return threeViewWidget->GetROIList();
        });
    QObject::connect(titleBarWidget, &TitleBarWidget::roiVisibilityChanged,
        threeViewWidget, &ThreeViewWidget::SetROIVisible);

    return app.exec();
}