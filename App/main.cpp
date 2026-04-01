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

    QString exeDir = QCoreApplication::applicationDirPath();
    QString logPath = exeDir + "/memory_leak_report.txt";
    std::wstring wlog = logPath.toStdWString();
    VLDSetReportOptions(VLD_OPT_REPORT_TO_FILE, wlog.c_str());

    QString path = "Chest1\\CT";
    std::string rsPath = "Chest1\\CT\\RS1.2.752.243.1.1.20240509084617335.3000.36570.dcm";

    // ===== 数据加载 =====
    DicomLoader loader;
    auto vtkImage = loader.Load(path.toStdString());

    // ===== UI 创建 =====
    StartWidget startWidget;
    ThreeViewWidget* threeViewWidget = new ThreeViewWidget(&startWidget);
    TitleBarWidget* titleBarWidget = new TitleBarWidget(&startWidget);
    LeftToolWidget* leftToolWidget = new LeftToolWidget(&startWidget);

    // ===== 数据注入 =====
    threeViewWidget->SetImageData(vtkImage);
    threeViewWidget->LoadRtStruct(rsPath);

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

    // ===== 信号连接（只做连接，不做逻辑） =====

    // 工具栏 → 交互模式
    QObject::connect(titleBarWidget, &TitleBarWidget::requestEnableDistanceMeasurement,
        threeViewWidget, &ThreeViewWidget::setModeToMeasurement);
    QObject::connect(titleBarWidget, &TitleBarWidget::requestEnableAngleMeasurement,
        threeViewWidget, &ThreeViewWidget::setModeToMeasurement);
    QObject::connect(titleBarWidget, &TitleBarWidget::requestEnableNormalMode,
        threeViewWidget, &ThreeViewWidget::setModeToMeasurement);
    QObject::connect(titleBarWidget, &TitleBarWidget::requestRoiNormalMode,
        threeViewWidget, &ThreeViewWidget::setModeToMeasurement);
    QObject::connect(titleBarWidget, &TitleBarWidget::requestFreehandROIMode,
        threeViewWidget, &ThreeViewWidget::setModeToMeasurement);
    QObject::connect(titleBarWidget, &TitleBarWidget::requestCrosshairRulerMode,
        threeViewWidget, &ThreeViewWidget::setModeToMeasurement);
    QObject::connect(titleBarWidget, &TitleBarWidget::requestMode,
        threeViewWidget, &ThreeViewWidget::setModeToMeasurement);

    // 工具栏 → 重置视图
    QObject::connect(titleBarWidget, &TitleBarWidget::requestResetViews,
        threeViewWidget, &ThreeViewWidget::ResetAllViews);

    // 图像拖拽 → 左侧面板
    QObject::connect(threeViewWidget, &ThreeViewWidget::imageDragUpdated,
        leftToolWidget, &LeftToolWidget::OnDragUpdated);
    QObject::connect(threeViewWidget, &ThreeViewWidget::imageDragReset,
        leftToolWidget, &LeftToolWidget::OnDragReset);

    // DICOM 元数据 → 左侧面板
    QMap<QString, QString> metadata = DicomMetadataExtractor::extractFromDirectory(path);
    leftToolWidget->SetDicomMetadata(metadata);

    // ★ 轮廓列表：注入数据提供者 + 连接可见性变化
    titleBarWidget->SetContourListProvider([threeViewWidget]() {
        return threeViewWidget->GetROIList();
        });

    QObject::connect(titleBarWidget, &TitleBarWidget::roiVisibilityChanged,
        threeViewWidget, &ThreeViewWidget::SetROIVisible);

    return app.exec();
}