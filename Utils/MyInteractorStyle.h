#ifndef MYINTERACTORSTYLE_H
#define MYINTERACTORSTYLE_H

#include <vtkInteractorStyleImage.h>
#include <vtkImageViewer2.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTextActor.h>
#include <vtkImageData.h>
#include <vtkImageActor.h>
#include <vtkPropPicker.h>
#include <vtkCellPicker.h>
#include <vtkSmartPointer.h>
#include <string>
#include <qdebug.h>

// 自定义交互样式：滚轮切片浏览 + 鼠标显示坐标/像素值
class MyInteractorStyle : public vtkInteractorStyleImage
{
public:
    static MyInteractorStyle* New();
    vtkTypeMacro(MyInteractorStyle, vtkInteractorStyleImage);

    // 设置要控制的 ImageViewer
    void SetImageViewer(vtkImageViewer2* v);
    void SetImageActor(vtkImageActor* a) { ImageActor = a; }
    void SetTextActor(vtkTextActor* actor) { this->TextActor = actor; }
    void SetDirection(double d[9]) { memcpy(Direction, d, sizeof(double) * 9); }
    void SetOrigin(double o[3]) { memcpy(Origin, o, sizeof(double) * 3); }

    // 滚轮向前：下一层
    void OnMouseWheelForward() override;

    // 滚轮向后：上一层
    void OnMouseWheelBackward() override;

    // 鼠标移动：显示坐标/像素值
    void OnMouseMove() override;

private:
    vtkImageViewer2* Viewer = nullptr;
    vtkTextActor* TextActor = nullptr;
    vtkImageActor* ImageActor = nullptr;
    double Direction[9];
    double Origin[3];


};

#endif // MYINTERACTORSTYLE_H
