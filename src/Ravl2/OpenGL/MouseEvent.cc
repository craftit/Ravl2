//
// Created by charles galambos on 26/01/2025.
//

#include <GLFW/glfw3.h>
#include "Ravl2/OpenGL/MouseEvent.hh"

namespace Ravl2 {


  MouseEvent::MouseEvent(MouseEventTypeT eventType,RealT col,RealT row,int buttonState,int buttonChanged,int keyModState)
    : mType(eventType),
      x(col),
      y(row),
      state((buttonState << 8) | keyModState),
      change(buttonChanged),
      time(0),
      m_doublePress(false)
  {
  }

  bool MouseEvent::IsPressed(int buttonNo) const {
    if(buttonNo < 0 || buttonNo >= MaxButtons()) {
      return false;
    }
    return (state & (1 << (8 + buttonNo))) != 0;
  }


  bool MouseEvent::HasChanged(int buttonNo) const {
    if(buttonNo < 0 || buttonNo >= MaxButtons()) {
      return false;
    }
    return (change & (1 << buttonNo)) != 0;
  }


  bool MouseEvent::IsShift() const {
    return (state & GLFW_MOD_SHIFT) != 0;
  }


  bool MouseEvent::IsCntrl() const {
    return (state & GLFW_MOD_CONTROL) != 0;
  }


  bool MouseEvent::IsLock() const {
    return (state & GLFW_MOD_NUM_LOCK) != 0;
  }

  bool MouseEvent::IsAlt() const {
    return (state & GLFW_MOD_ALT) != 0;
  }


  bool MouseEvent::IsMod5() const {
#if 0
    return (state & GDK_MOD2_MASK) != 0;
#else
    return false;
#endif
  }

  //: Sixth modifier key.

  bool MouseEvent::IsMod6() const {
#if 0
    return (state & GDK_MOD3_MASK) != 0;
#else
    return false;
#endif
  }

  bool MouseEvent::IsMod7() const {
#if 0
    return (state & 0) != 0;
#else
    return false;
#endif
  }

  bool MouseEvent::IsMod8() const {
#if 0
    return (state & GDK_MOD5_MASK) != 0;
#else
    return false;
#endif
  }


  std::ostream &operator<<(std::ostream &s,const MouseEvent &me) {
    s << " x:" << me.Row() << " y:" << me.Col();
    return s;
  }

} // Ravl2