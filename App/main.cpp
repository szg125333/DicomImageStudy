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
#include "Controller/ThreeViewController.h"    // 控制器实现
#include "Interface/IViewRenderer.h"
#include "Renderer/OverlayManager/IOverlayManager.h"

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	QString exeDir = QCoreApplication::applicationDirPath();
	QString logPath = exeDir + "/memory_leak_report.txt";
	std::wstring wlog = logPath.toStdWString();
	VLDSetReportOptions(VLD_OPT_REPORT_TO_FILE, wlog.c_str());

	//QString path = "C:\\Workspace\\testData\\PositionTest\\HFS\\CT";
	//QString path = "C:\\Workspace\\testData\\FZJ";
	//QString path = "C:\\Workspace\\testData\\registrationData\\Chest1\\CT";
	//std::string rsPath = "C:\\Workspace\\testData\\registrationData\\Chest1\\CT\\RS1.2.752.243.1.1.20240509084617335.3000.36570.dcm";

	QString path = "Chest1\\CT";
	std::string rsPath = "Chest1\\CT\\RS1.2.752.243.1.1.20240509084617335.3000.36570.dcm";

	DicomLoader loader;
	auto vtkImage = loader.Load(path.toStdString());

	ImageOrientationResampler resampler;
	std::vector<std::string> dicomFiles = resampler.loadDicomSeries(path);
	dicomFiles = resampler.SortDicomFiles(dicomFiles);
	auto cbctImage = resampler.ReadDicomSeries(dicomFiles);    // 读取 CBCT 序列

	double  origin1[3];
	origin1[0] = cbctImage->GetOrigin()[0];
	origin1[1] = cbctImage->GetOrigin()[1];
	origin1[2] = cbctImage->GetOrigin()[2];


	using ITKToVTKFilterType = itk::ImageToVTKImageFilter<itk::Image<short, 3>>;
	auto itkToVtk = ITKToVTKFilterType::New();
	itkToVtk->SetInput(cbctImage);
	itkToVtk->Update();

	double  origin2[3];
	origin2[0] = itkToVtk->GetOutput()->GetOrigin()[0];
	origin2[1] = itkToVtk->GetOutput()->GetOrigin()[1];
	origin2[2] = itkToVtk->GetOutput()->GetOrigin()[2];

	StartWidget startWidget;

	// 创建子部件
	ThreeViewWidget* threeViewWidget = new ThreeViewWidget(&startWidget);
	TitleBarWidget* titleBarWidget = new TitleBarWidget(&startWidget);
	LeftToolWidget* leftToolWidget = new LeftToolWidget(&startWidget);

	threeViewWidget->SetImageData(vtkImage);
	threeViewWidget->LoadRtStruct(rsPath);

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

	QObject::connect(titleBarWidget, &TitleBarWidget::requestEnableDistanceMeasurement, threeViewWidget, &ThreeViewWidget::setModeToMeasurement);
	QObject::connect(titleBarWidget, &TitleBarWidget::requestEnableAngleMeasurement, threeViewWidget, &ThreeViewWidget::setModeToMeasurement);
	QObject::connect(titleBarWidget, &TitleBarWidget::requestEnableNormalMode, threeViewWidget, &ThreeViewWidget::setModeToMeasurement);
	QObject::connect(titleBarWidget, &TitleBarWidget::requestRoiNormalMode, threeViewWidget, &ThreeViewWidget::setModeToMeasurement);
	QObject::connect(titleBarWidget, &TitleBarWidget::requestResetViews, threeViewWidget, &ThreeViewWidget::ResetAllViews);
	QObject::connect(titleBarWidget, &TitleBarWidget::requestFreehandROIMode, threeViewWidget, &ThreeViewWidget::setModeToMeasurement);
	QObject::connect(titleBarWidget, &TitleBarWidget::requestCrosshairRulerMode, threeViewWidget, &ThreeViewWidget::setModeToMeasurement);

	QObject::connect(titleBarWidget, &TitleBarWidget::requestMode, threeViewWidget, &ThreeViewWidget::setModeToMeasurement);
	QObject::connect(threeViewWidget, &ThreeViewWidget::imageDragUpdated,
		leftToolWidget, &LeftToolWidget::OnDragUpdated);

	QObject::connect(threeViewWidget, &ThreeViewWidget::imageDragReset,
		leftToolWidget, &LeftToolWidget::OnDragReset);
	QMap<QString, QString> metadata = DicomMetadataExtractor::extractFromDirectory(path);
	leftToolWidget->SetDicomMetadata(metadata);


	// TitleBarWidget 请求列表 → 从 ThreeViewWidget 获取
	QObject::connect(titleBarWidget, &TitleBarWidget::requestContourList, [threeViewWidget, titleBarWidget]() {
		auto* mgr = threeViewWidget->getController()->GetRenderer(0)->GetOverlayManager();
		if (mgr) {
			titleBarWidget->UpdateContourList(mgr->GetROIList());
		}
		});

	// TitleBarWidget 可见性变化 → 更新所有视图
	QObject::connect(titleBarWidget, &TitleBarWidget::roiVisibilityChanged,
		[threeViewWidget](int roiNumber, bool visible) {
			auto* controller = threeViewWidget->getController();
			for (int i = 0; i < 3; ++i) {
				auto* mgr = controller->GetRenderer(i)->GetOverlayManager();
				if (mgr) mgr->SetROIVisible(roiNumber, visible);
			}
			for (int i = 0; i < 3; ++i) {
				auto vt = static_cast<ViewType>(i);
				controller->GetRenderer(i)->GetOverlayManager()->OnSliceChanged(vt, controller->GetSlice(vt));
				controller->GetRenderer(i)->RequestRender();
			}
		});

	return app.exec();
}

