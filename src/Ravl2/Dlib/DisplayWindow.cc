//
// Created by charles on 02/09/24.
//

#include "Ravl2/Dlib/DisplayWindow.hh"


namespace Ravl2::DLibIO
{

  DisplayWindow::DisplayWindow()
      : m_queue(16)
  {
    start();
  }

  DisplayWindow::~DisplayWindow()
  {
    m_terminate = true;
    if(m_thread.joinable())
      m_thread.join();
  }

  void
  DisplayWindow::start()
  {
    if(!m_thread.joinable()) {
      m_thread = std::thread(&DisplayWindow::run, this);
    }
  }


  void
  DisplayWindow::run()
  {
    SPDLOG_INFO("DisplayWindow: thread started");
    while(!m_terminate)
    {
      std::function<void(DisplayWindow &win)> f;
      if(m_queue.popWait(f,0.5)) {
        f(*this);
      }
    }
    SPDLOG_INFO("DisplayWindow: thread terminated");
  }

}