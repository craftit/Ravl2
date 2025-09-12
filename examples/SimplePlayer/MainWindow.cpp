//
// Created on September 12, 2025
//

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "VideoPlayer.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QResizeEvent>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , videoPlayer(new VideoPlayer(this))
{
    ui->setupUi(this);

    // Set up video display widget
    ui->videoWidget->setMinimumSize(320, 240);

    // Connect video player signals to UI
    connect(videoPlayer.get(), &VideoPlayer::durationChanged, this, [this](int64_t duration) {
        ui->positionSlider->setMaximum(static_cast<int>(duration / 1000000)); // Convert to seconds
        ui->totalTimeLabel->setText(formatTime(duration));
    });

    connect(videoPlayer.get(), &VideoPlayer::positionChanged, this, [this](int64_t position) {
        if (!ui->positionSlider->isSliderDown()) {
            ui->positionSlider->setValue(static_cast<int>(position / 1000000)); // Convert to seconds
        }
        ui->currentTimeLabel->setText(formatTime(position));
    });

    connect(videoPlayer.get(), &VideoPlayer::videoInfoChanged, this, [this](const QString &info) {
        ui->statusBar->showMessage(info);
    });

    // Set up timer for UI updates
    connect(&updateTimer, &QTimer::timeout, this, &MainWindow::updatePlaybackState);
    updateTimer.start(100); // Update every 100ms

    // Initialize UI
    updateUI();
}

MainWindow::~MainWindow()
{
    updateTimer.stop();
}

void MainWindow::openFile(const QString &filePath)
{
    if (videoPlayer->openFile(filePath)) {
        setWindowTitle(QString("SimplePlayer - %1").arg(filePath));
        updateUI();
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Could not open video file."));
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateVideoFrame();
}

void MainWindow::on_actionOpen_triggered()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Video File"), QString(),
        tr("Video Files (*.mp4 *.avi *.mkv *.mov);;All Files (*)"));

    if (!filePath.isEmpty()) {
        openFile(filePath);
    }
}

void MainWindow::on_playPauseButton_clicked()
{
    if (isPlaying) {
        videoPlayer->pause();
    } else {
        videoPlayer->play();
    }
    isPlaying = !isPlaying;
    updateUI();
}

void MainWindow::on_stopButton_clicked()
{
    videoPlayer->stop();
    isPlaying = false;
    updateUI();
}

void MainWindow::on_positionSlider_sliderMoved(int position)
{
    videoPlayer->seek(position * 1000000); // Convert to microseconds
}

void MainWindow::on_nextFrameButton_clicked()
{
    videoPlayer->nextFrame();
    updateUI();
}

void MainWindow::on_prevFrameButton_clicked()
{
    videoPlayer->previousFrame();
    updateUI();
}

void MainWindow::updatePlaybackState()
{
    if (isPlaying) {
        updateVideoFrame();
    }
}

void MainWindow::updateVideoFrame()
{
    QPixmap frame = videoPlayer->getCurrentFrame();
    if (!frame.isNull()) {
        // Scale frame to fit the video widget while maintaining aspect ratio
        QSize widgetSize = ui->videoWidget->size();
        QPixmap scaledFrame = frame.scaled(
            widgetSize,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );

        ui->videoWidget->setPixmap(scaledFrame);
    }
}

void MainWindow::updateUI()
{
    ui->playPauseButton->setText(isPlaying ? tr("Pause") : tr("Play"));

    //! Enable playPauseButton when a file is loaded
    ui->playPauseButton->setEnabled(videoPlayer->isFileLoaded());

    ui->nextFrameButton->setEnabled(!isPlaying && videoPlayer->isFileLoaded());
    ui->prevFrameButton->setEnabled(!isPlaying && videoPlayer->isFileLoaded());
    ui->positionSlider->setEnabled(videoPlayer->isFileLoaded());
    ui->stopButton->setEnabled(isPlaying || videoPlayer->getPosition() > 0);

    updateVideoFrame();
}

QString MainWindow::formatTime(int64_t microseconds) const
{
    int totalSeconds = static_cast<int>(microseconds / 1000000);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    int ms = static_cast<int>((microseconds % 1000000) / 10000); // Show only 2 digits

    if (hours > 0) {
        return QString("%1:%2:%3.%4")
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
            .arg(ms, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2.%3")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
            .arg(ms, 2, 10, QChar('0'));
    }
}
