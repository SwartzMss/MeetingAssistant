#include <QApplication>
#include <QCommandLineParser>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(true);
    
    MainWindow window;
    window.show();
    
    return app.exec();
} 