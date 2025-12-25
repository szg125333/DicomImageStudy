#ifndef MYINTERACTORSTYLE_H
#define MYINTERACTORSTYLE_H

#include <vtkInteractorStyleImage.h>
#include <vtkImageViewer2.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTextActor.h>
#include <vtkimageData.h>
#include <vtkImageActor.h>
#include <vtkPropPicker.h>
#include <vtkCellPicker.h>
#include <vtkSmartPointer.h>
#include <sstream>   // 必须！提供 std::stringstream
#include <string>    // 通常也需要
#include <qdebug.h>    // 通常也需要
// 自定义交互样式：滚轮切片浏览
class MyInteractorStyle : public vtkInteractorStyleImage
{
public:
    static MyInteractorStyle* New();
    vtkTypeMacro(MyInteractorStyle, vtkInteractorStyleImage);

    // 设置要控制的 ImageViewer
    void SetImageViewer(vtkImageViewer2* v);
    void SetImageActor(vtkImageActor* a) { ImageActor = a; }
    // 滚轮向前：下一层
    void OnMouseWheelForward() override;

    // 滚轮向后：上一层
    void OnMouseWheelBackward() override;

    void SetTextActor(vtkTextActor* actor) { this->TextActor = actor; }

    virtual void OnMouseMove() override
    {
        int pos[2];
        this->GetInteractor()->GetEventPosition(pos);

        vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
        picker->SetTolerance(0.0005);
        picker->AddPickList(this->ImageActor);
        picker->PickFromListOn();

        // 关键：绑定 viewer 的 image actor
        picker->Pick(pos[0], pos[1], 0, this->Viewer->GetRenderer());

        double world[3];
        picker->GetPickPosition(world);

        vtkImageData* image = this->Viewer->GetInput();
        double spacing[3], origin[3];
        image->GetSpacing(spacing);
        image->GetOrigin(origin);

        int ix = (world[0] - origin[0]) / spacing[0];
        int iy = (world[1] - origin[1]) / spacing[1];
        int iz = this->Viewer->GetSlice();

        int* dims = image->GetDimensions();

        if (ix >= 0 && ix < dims[0] && iy >= 0 && iy < dims[1])
        {
            double pixelValue = image->GetScalarComponentAsDouble(ix, iy, iz, 0);

            std::string text =
                "X: " + std::to_string(ix) +
                "  Y: " + std::to_string(iy) +
                "  Slice: " + std::to_string(iz) +
                "  Value: " + std::to_string(pixelValue);

            this->TextActor->SetInput(text.c_str());
            this->Viewer->Render();
        }

        vtkInteractorStyleImage::OnMouseMove();
    }

private:
    vtkImageViewer2* Viewer = nullptr;
    vtkTextActor* TextActor = nullptr;
    vtkImageActor* ImageActor = nullptr;


};

#endif // MYINTERACTORSTYLE_H
