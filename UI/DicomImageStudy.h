#ifndef DICOMIMAGESTUDY_H
#define DICOMIMAGESTUDY_H

#include <QMainWindow>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>
#include <vtkImageViewer2.h>
#include "Utils/MyInteractorStyle.h"

namespace Ui {
class DicomImageStudyClass;
}


class DicomImageStudy : public QMainWindow
{
    Q_OBJECT

public:
    explicit DicomImageStudy(QWidget *parent = nullptr);
    ~DicomImageStudy();

    void loadDicom(const std::string& folder);
    void showPNG(const std::string& pngFile);
private:
    Ui::DicomImageStudyClass *ui;

    // VTK ¿Ø¼þºÍ viewer
    QVTKOpenGLNativeWidget* vtkWidget;
    vtkSmartPointer<vtkImageViewer2> viewer;
};

#endif // DICOMIMAGESTUDY_H
