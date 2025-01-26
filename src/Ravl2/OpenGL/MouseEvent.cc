//
// Created by charles galambos on 26/01/2025.
//

#include <GLFW/glfw3.h>
#include "Ravl2/OpenGL/MouseEvent.hh"

namespace Ravl2 {
  //: Constructor.

  MouseEvent::MouseEvent(RealT nx,RealT ny,int nstate,int _changed,size_t _time)
    : x(nx),
      y(ny),
      state(nstate),
      change(_changed),
      time(_time),
      m_doublePress(false)
  {}

  //: Constructor.

#if 0
  MouseEventC::MouseEventC(GdkEventButton &ev)
    : x(ev.x),
      y(ev.y),
      state(ev.state),
      change(0),
      time(ev.time),
      m_doublePress(ev.type == GDK_2BUTTON_PRESS)
  {
    change = 1 << (ev.button - 1);
    ONDEBUG(std::cerr << "MouseEventC::MouseEventC(), Button: " << ev.button << "\n");
  }

  //: Constructor.

  MouseEventC::MouseEventC(GdkEventMotion &event)
    : x(event.x),
      y(event.y),
      state(event.state),
      change(0),
      time(event.time)
  {
    if (event.is_hint) {
      int ix,iy;
      gdk_window_get_pointer (event.window, &ix, &iy,(GdkModifierType *) &state);
      x = ix;
      y = iy;
    } else {
      x = event.x;
      y = event.y;
      state = event.state;
    }
  }
#endif

  //: Test if a button is pessed.

  bool MouseEvent::IsPressed(int buttonNo) const {
    if(buttonNo < 0 || buttonNo >= MaxButtons()) {
      return false;
    }
    return (state & (1 << (8 + buttonNo))) != 0;
  }

  //: Has button changed state ?

  bool MouseEvent::HasChanged(int buttonNo) const {
    if(buttonNo < 0 || buttonNo >= MaxButtons()) {
      return false;
    }
    return (change & (1 << buttonNo)) != 0;
  }


  //: Is shift down ?

  bool MouseEvent::IsShift() const {
    return (state & GLFW_MOD_SHIFT) != 0;
  }

  //: Is control down ?

  bool MouseEvent::IsCntrl() const {
    return (state & GLFW_MOD_CONTROL) != 0;
  }

  //: Is the caps lock key down ?

  bool MouseEvent::IsLock() const {
    return (state & GLFW_MOD_NUM_LOCK) != 0;
  }

  //: Is the alt key pressed?

  bool MouseEvent::IsAlt() const {
    return (state & GLFW_MOD_ALT) != 0;
  }

  //: Forth modifier key.

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

  //: Seventh modifier key.

  bool MouseEvent::IsMod7() const {
#if 0
    return (state & 0) != 0;
#else
    return false;
#endif
  }

  //: Eight modifier key.

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