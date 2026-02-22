#include "SensorDashboard.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    SensorDashboard dashboard;
    dashboard.show();

    return app.exec();
}
