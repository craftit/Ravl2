//
// Created by charles on 30/03/23.
//

#pragma once

#include <functional>
#include <spdlog/spdlog.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/opencv.h>

#include "Ravl2/ThreadedQueue.hh"

namespace Ravl2::DLibIO
{

  //! Display data from the vision system

  class DisplayWindow
  {
  public:
    //! Constructor, this will start the display thread
    DisplayWindow();

    //! Destructor
    ~DisplayWindow();

    //! Don't call outside the vision thread
    template<typename ImageT>
    void display(const ImageT &image)
    {
      m_win.clear_overlay();
      m_win.set_image(image);
    }

    //! Don't call outside the vision thread
    template<typename ImageT, typename OverlayT>
    void display(const ImageT &image, const OverlayT &overlay)
    {
      m_win.clear_overlay();
      m_win.set_image(image);
      m_win.add_overlay(overlay);
    }

    //! @brief Access display window
    //! Don't call outside the vision thread
    dlib::image_window &window()
    { return m_win; }


    //! Queue a function to be called in the display thread
    void queue(std::function<void(DisplayWindow &win)> &&f)
    {
      if(!m_queue.tryPush(std::move(f))) {
        SPDLOG_WARN("DisplayWindow: queue full, dropping frame");
      }
    }

  protected:
    //! Start the display thread
    void start();

    void run();

    ThreadedQueue<std::function<void(DisplayWindow &win)> > m_queue {16};

    std::atomic<bool> m_terminate = false;
    std::thread m_thread;
    dlib::image_window m_win;
  };

}
