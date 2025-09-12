//
// Created on September 12, 2025
//

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("SimplePlayer");
    app.setApplicationVersion("0.1.0");

    // Set up command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Simple video player using Ravl2");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "The video file to open.");
    parser.process(app);

    // Create main window
    MainWindow mainWindow;
    mainWindow.show();

    // If a file was specified on the command line, try to open it
    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        const QString fileName = args.first();
        QFileInfo fileInfo(fileName);
        if (fileInfo.exists() && fileInfo.isFile()) {
            mainWindow.openFile(fileName);
        }
    }

    return app.exec();
}
