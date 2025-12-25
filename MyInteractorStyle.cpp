#include "MyInteractorStyle.h"
#include <vtkObjectFactory.h>
#include <qdebug.h>

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
