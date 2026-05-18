#include "main_window.h"
#include <QApplication>
#include <QLocale>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);


    QLocale::setDefault(QLocale::c());

    MainWindow window;
    window.show();
    return app.exec();
}
