#include "mainwindow.hpp"

#include <QApplication>

int main(int argc, char *argv[])
{
    QLoggingCategory::setFilterRules("*.warning=false");
    qSetMessagePattern("[%{time hh:mm:ss}] %{message}\n");

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
