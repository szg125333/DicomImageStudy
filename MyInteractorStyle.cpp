#include "MyInteractorStyle.h"

#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkImageData.h>

vtkStandardNewMacro(MyInteractorStyle);

void MyInteractorStyle::SetImageViewer(vtkImageViewer2* v)
{
    this->Viewer = v;
}

void MyInteractorStyle::OnMouseWheelForward()
{
    if (!this->Viewer) return;

    int slice = this->Viewer->GetSlice();
    qDebug() << "slice:" << slice;
    if (slice < this->Viewer->GetSliceMax()) {
        this->Viewer->SetSlice(slice + 1);
        this->Viewer->Render();
    }
}

void MyInteractorStyle::OnMouseWheelBackward()
{
    if (!this->Viewer) return;

    int slice = this->Viewer->GetSlice();
    qDebug() << "slice:" << slice;
    if (slice > this->Viewer->GetSliceMin()) {
        this->Viewer->SetSlice(slice - 1);
        this->Viewer->Render();
    }
}

void MyInteractorStyle::OnMouseMove()
{
    if (!Viewer || !ImageActor || !TextActor) {
        vtkInteractorStyleImage::OnMouseMove();
        return;
    }

    int pos[2];
    this->GetInteractor()->GetEventPosition(pos);

    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.0005);
    picker->InitializePickList();
    picker->AddPickList(this->ImageActor);
    picker->PickFromListOn();

    picker->Pick(pos[0], pos[1], 0, this->Viewer->GetRenderer());

    double world[3];
    picker->GetPickPosition(world);

    vtkImageData* image = this->Viewer->GetInput();
    double spacing[3];
    image->GetSpacing(spacing);

    // -----------------------------
    // 1. 计算 index（必须）
    // -----------------------------
    int ix = (world[0] - Origin[0]) / spacing[0];
    int iy = (world[1] - Origin[1]) / spacing[1];
    int iz = this->Viewer->GetSlice();

    int* dims = image->GetDimensions();

    // -----------------------------
    // 2. 计算物理坐标（你问的代码就在这里）
    // -----------------------------
    double physical[3];
    for (int r = 0; r < 3; r++)
    {
        physical[r] =
            Origin[r]
            + Direction[r * 3 + 0] * (ix * spacing[0])
            + Direction[r * 3 + 1] * (iy * spacing[1])
            + Direction[r * 3 + 2] * (iz * spacing[2]);
    }

    // -----------------------------
    // 3. 构造显示文本
    // -----------------------------
    std::string text;

    text += "Phys(mm): "
        + std::to_string(physical[0]) + ", "
        + std::to_string(physical[1]) + ", "
        + std::to_string(physical[2]) + "\n";

    if (ix >= 0 && ix < dims[0] && iy >= 0 && iy < dims[1] && iz >= 0 && iz < dims[2])
    {
        double pixelValue = image->GetScalarComponentAsDouble(ix, iy, iz, 0);

        text += "Index: "
            + std::to_string(ix) + ", "
            + std::to_string(iy) + ", "
            + std::to_string(iz)
            + "  Value: " + std::to_string(pixelValue);
    }
    else
    {
        text += "Index: (out of range)";
    }

    this->TextActor->SetInput(text.c_str());
    this->Viewer->Render();

    vtkInteractorStyleImage::OnMouseMove();
}

