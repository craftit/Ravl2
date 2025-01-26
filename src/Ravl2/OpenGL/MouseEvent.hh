//
// Created by charles galambos on 26/01/2025.
//

#pragma once

#include <GL/glut.h>
#include "Ravl2/Types.hh"

namespace Ravl2 {

  //! userlevel=Normal
  //: Mouse event information.
  
  class MouseEvent {
  public:
    using RealT = float;

    MouseEvent() = default;

    //! Constructor.
    explicit MouseEvent(RealT col,RealT row,int state = 0,int changed = 0,size_t time = 0);

    //: Get row number of position.
    [[nodiscard]] RealT Row() const { return y; }

    //: Get column number of position.
    [[nodiscard]] RealT Col() const { return x; }

    //: Get position.
    // Position of mouse click in RAVL co-ordinates (row, col).
    [[nodiscard]] Index<2> At() const { return toIndex(y,x); }

    //: Test if a button is pressed.
    [[nodiscard]] bool IsPressed(int buttonNo = 0) const;

    [[nodiscard]] bool IsDoublePressed() const
    { return m_doublePress; }

    //: Has button changed state ?
    [[nodiscard]] bool HasChanged(int buttonNo = 0) const;

    //: Maximum buttons available.
    [[nodiscard]] static int MaxButtons() { return 5; }

    //: Is shift down ?
    [[nodiscard]] bool IsShift() const;

    //: Is control down ?
    [[nodiscard]] bool IsCntrl() const;

    //: Is the caps lock key down ?
    [[nodiscard]] bool IsLock() const;

    //: Is the alt key pressed?
    [[nodiscard]] bool IsAlt() const;

    //! Forth modifier key.
    [[nodiscard]] bool IsMod5() const;

    //! Sixth modifier key.
    [[nodiscard]] bool IsMod6() const;

    //! Seventh modifier key.
    // Can be 'Windows Key'
    [[nodiscard]] bool IsMod7() const;

    //! Eight modifier key.
    // Can be 'Alt Gr'
    [[nodiscard]] bool IsMod8() const;

    //! Access raw state value.
    [[nodiscard]] int RawState() const
    { return state; }

    //! Raw button changed bits.
    [[nodiscard]] int RawChanged() const
    { return change; }

    //! Time of event (from gdk)
    [[nodiscard]] size_t Time() const
    { return time; }
  protected:
    RealT x = 0;
    RealT y = 0;   // Current position in GTK coords.
    int mKeyModifiers = 0; // Key modifiers.
    int state = 0;  // Current state.
    int change = 0; // Change flags.
    size_t time = 0;  // Time of event.
    bool m_doublePress = false;
  };
  
  std::ostream &operator<<(std::ostream &,const MouseEvent &me);
  //: Print description of mouse event to stream.

} // Ravl2
