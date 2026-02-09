// CTViewer.h
#ifndef CTVIEWER_H
#define CTVIEWER_H

#include <string>
#include <itkImage.h>
#include <itkImageSeriesReader.h>
#include <itkImageToVTKImageFilter.h>
#include <vtkImageData.h>

class CTViewer
{
public:
    typedef itk::Image<short, 3> InputImageType;
    typedef itk::ImageSeriesReader<InputImageType> ReaderType;
    typedef itk::ImageToVTKImageFilter<InputImageType> ConnectorType;

    explicit CTViewer(const std::string& dicomFolder);
    vtkSmartPointer<vtkImageData> getVTKImage();
    std::vector<std::string> GetAllCTFilePaths(const std::string& dicomFolder);
    void getDirection(double out[9]) const {
        memcpy(out, direction, sizeof(double) * 9);
    }

    void getOrigin(double out[3]) const {
        memcpy(out, origin, sizeof(double) * 3);
    }

private:
    std::string dicomFolder="";
    itk::ImageToVTKImageFilter<InputImageType>::Pointer connector;
    double direction[9];   // 3×3 方向矩阵
    double origin[3];      // DICOM 原点 (0020,0032)

};

#endif // CTVIEWER_H
