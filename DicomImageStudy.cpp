#include "DicomImageStudy.h"
#include "ui_DicomImageStudy.h"
#include "CTViewer.h"

#include <QVBoxLayout>
#include <vtkRenderer.h>
#include <qdebug.h>
#include <vtkMetaImageWriter.h>
#include <vtkInteractorStyleImage.h>
#include <vtkRenderWindow.h>
#include <vtkPointData.h>
#include <vtkCamera.h>
#include <vtkPNGReader.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageMapToWindowLevelColors.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>

DicomImageStudy::DicomImageStudy(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::DicomImageStudyClass)
{
    ui->setupUi(this);

    // 在 UI 界面里放一个 VTK 控件
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    vtkWidget = new QVTKOpenGLNativeWidget(this);
    vtkWidget->setFocusPolicy(Qt::StrongFocus); 
    vtkWidget->setFocus();
    vtkWidget->setRenderWindow(renderWindow);
    ui->verticalLayout->addWidget(vtkWidget);  // 假设 UI 里有一个 verticalLayout

    // 初始化 VTK viewer
    viewer = vtkSmartPointer<vtkImageViewer2>::New();
    viewer->SetRenderWindow(vtkWidget->renderWindow());
}

DicomImageStudy::~DicomImageStudy()
{
    delete ui;
}

void DicomImageStudy::loadDicom(const std::string& folder)
{
    CTViewer ct(folder);
    vtkSmartPointer<vtkImageData> vtkImage = ct.getVTKImage();  // 从 CTViewer 获取 VTK 图像

    //vtkSmartPointer<vtkMetaImageWriter> writer = vtkSmartPointer<vtkMetaImageWriter>::New();
    //// 这一句如果你要“物理坐标 = DICOM 坐标”，其实不建议写死为 0,0,0
    //// vtkImage->SetOrigin(0, 0, 0);

    //writer->SetInputData(vtkImage);
    //writer->SetFileName("CT_Saved.mhd"); // .raw 文件会自动生成在同目录
    //writer->SetCompression(false);
    //writer->Write();

    if (vtkImage)
    {
        if (!vtkImage->GetPointData() || !vtkImage->GetPointData()->GetScalars()) {
            qDebug() << "Error: vtkImage has no scalar data!";
            return;
        }
        double range[2];
        vtkImage->GetScalarRange(range);
        qDebug() << "range:" << range[0] << " " << range[1];

        // 左上角文字
        vtkSmartPointer<vtkTextActor> textActor = vtkSmartPointer<vtkTextActor>::New();
        textActor->SetInput("X: -, Y: -, Value: -");
        textActor->GetTextProperty()->SetFontSize(20);
        textActor->GetTextProperty()->SetColor(1.0, 1.0, 0.0); // 黄色
        textActor->SetDisplayPosition(10, 10); // 左上角
        viewer->GetRenderer()->AddActor2D(textActor);

        vtkSmartPointer<vtkImageMapToWindowLevelColors> wl =
            vtkSmartPointer<vtkImageMapToWindowLevelColors>::New();
        wl->SetInputData(vtkImage);
        wl->SetWindow(2000);
        wl->SetLevel(500);
        wl->Update();
        viewer->SetInputConnection(wl->GetOutputPort());

        viewer->SetSliceOrientationToXY();//设置切片方向
        int midSlice = (viewer->GetSliceMin() + viewer->GetSliceMax()) / 2;
        viewer->SetSlice(midSlice);

        // 自定义交互样式
        vtkSmartPointer<MyInteractorStyle> style = vtkSmartPointer<MyInteractorStyle>::New();
        style->SetImageViewer(viewer);
        style->SetTextActor(textActor);

        vtkImageActor* actor = viewer->GetImageActor();
        style->SetImageActor(actor);
        double direction[9];
        double origin[3];

        ct.getDirection(direction);
        ct.getOrigin(origin);

        style->SetDirection(direction);
        style->SetOrigin(origin);


        viewer->GetRenderWindow()->GetInteractor()->SetInteractorStyle(style);

        qDebug() << "SliceMin:" << viewer->GetSliceMin()
            << "SliceMax:" << viewer->GetSliceMax();
        //double* origin = vtkImage->GetOrigin();
        double* spacing = vtkImage->GetSpacing();
        qDebug() << "Origin:" << origin[0] << origin[1] << origin[2];
        qDebug() << "Spacing:" << spacing[0] << spacing[1] << spacing[2];

        viewer->Render();
        viewer->GetRenderer()->ResetCamera();
    }
}
