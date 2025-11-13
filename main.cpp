/////////////////////////////////////////////////////////////
// MAIN.CPP - Application Entry Point
/////////////////////////////////////////////////////////////

#include "smartrainharvest.h"
#include <QApplication>

int main(int argc, char* argv[])
{
    // Initialize the Qt application with command-line arguments
    QApplication a(argc, argv);

    // Create the main window instance
    SmartRainHarvest w;

    // Display the window in maximized mode
    w.showMaximized();

    // Start the Qt event loop and return the exit code when application closes
    return a.exec();
}