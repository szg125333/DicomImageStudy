#include "DicomImageStudy.h"
#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);


    DicomImageStudy window;
    window.resize(800, 600);

    //window.loadDicom("C:\\Workspace\\testData\\registrationData\\Head1\\CT");
    window.loadDicom("C:\\Workspace\\testData\\HFP");
    window.show();

    return app.exec();
}
