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

private:
    std::string dicomFolder="";
    itk::ImageToVTKImageFilter<InputImageType>::Pointer connector;

};

#endif // CTVIEWER_H
