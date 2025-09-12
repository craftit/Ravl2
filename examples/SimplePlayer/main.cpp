//
// Created on September 12, 2025
//

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QDir>
#include <QString>
#include <QDebug>
#include <string>
#include <filesystem>
#include "MainWindow.h"
#include "Ravl2/Resource.hh"
#include "Ravl2/config.hh"
#include "Ravl2/Logging.hh"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("SimplePlayer");
    app.setApplicationVersion("0.1.0");

    Ravl2::initializeLogging();

    //! Set up command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Simple video player using Ravl2");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "The video file to open.");
    parser.process(app);

    //! Create main window
    MainWindow mainWindow;
    mainWindow.show();

    //! Default video file to use if none is specified
    QString filePath;

    //! If a file was specified on the command line, try to open it
    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        filePath = args.first();
    } else {
        //! No file specified, use default "sample-5s.mp4" from resources
        std::string defaultFile = "sample-5s.mp4";

        //! Add the data path to resource paths like in exMediaContainer.cc
        Ravl2::addResourcePath("data", RAVL_SOURCE_DIR "/data");

        //! Try to find the file in resources
        std::string resourceFile = Ravl2::findFileResource("data", defaultFile);
        if (!resourceFile.empty()) {
            filePath = QString::fromStdString(resourceFile);
            qDebug() << "Using default sample video from resource:" << filePath;
        } else {
            //! If resource not found, check the data directory directly
            std::filesystem::path dataPath(RAVL_SOURCE_DIR);
            dataPath /= "data";
            dataPath /= defaultFile;

            if (std::filesystem::exists(dataPath)) {
                filePath = QString::fromStdString(dataPath.string());
                qDebug() << "Using default sample video:" << filePath;
            }
        }
    }

    //! If we have a valid file path, try to open it
    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath);
        if (fileInfo.exists() && fileInfo.isFile()) {
            mainWindow.openFile(filePath);
        } else {
            qWarning() << "File does not exist:" << filePath;
        }
    }

    return app.exec();
}
