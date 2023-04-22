#include <QApplication>
#include <QCommandLineParser>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication qdd(argc, argv);
    MainWindow w;
    QGuiApplication::setApplicationDisplayName(MainWindow::tr("QData Decoder"));
    QCommandLineParser commandLineParser;
    commandLineParser.addHelpOption();
    commandLineParser.addPositionalArgument(MainWindow::tr("[file]"), MainWindow::tr(""));
    commandLineParser.process(QCoreApplication::arguments());
    if (!commandLineParser.positionalArguments().isEmpty() && !w.loadFile(commandLineParser.positionalArguments().constFirst()))
    {
        return -1;
    }
    w.show();
    return qdd.exec();
}
