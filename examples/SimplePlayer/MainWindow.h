//
// Created on September 12, 2025
//

#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QString>
#include <memory>

namespace Ui
{
  class MainWindow;
}

class VideoPlayer;

//! Main window for the SimplePlayer application
class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  //! Constructor
  explicit MainWindow(QWidget *parent = nullptr);

  //! Destructor
  ~MainWindow() override;

  //! Open a video file
  void openFile(const QString &filePath);

protected:
  //! Handle resize events
  void resizeEvent(QResizeEvent *event) override;

private slots:
  //! Handle file open action
  void on_actionOpen_triggered();

  //! Handle play/pause button
  void on_playPauseButton_clicked();

  //! Handle stop button
  void on_stopButton_clicked();

  //! Handle slider position change
  void on_positionSlider_sliderMoved(int position);

  //! Handle frame step forward
  void on_nextFrameButton_clicked();

  //! Handle frame step backward
  void on_prevFrameButton_clicked();

  //! Update UI with current playback state
  void updatePlaybackState();

private:
  //! Update video display
  void updateVideoFrame();

  //! Update UI elements based on video state
  void updateUI();

  //! Format time display (HH:MM:SS.ms)
  QString formatTime(int64_t microseconds) const;

  std::unique_ptr<Ui::MainWindow> ui;
  std::unique_ptr<VideoPlayer> videoPlayer;
  QTimer updateTimer;
  bool isPlaying = false;
};
