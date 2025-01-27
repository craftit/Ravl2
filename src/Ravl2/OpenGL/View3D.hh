// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.Graphics.3D"
//! author="James Smith"

#pragma once

#include <mutex>
#include <shared_mutex>
#include "Ravl2/OpenGL/Canvas3D.hh"
#include "Ravl2/OpenGL/DObject3D.hh"


namespace Ravl2 {

  class MouseEvent;
  class GLWindow;

  //! @brief 3D Viewer widget.
  // <p>This class enables you to manipulate a 3D object as you view it, using the mouse.  </p>
  // <ul>
  // <li> Button 1 rotates about horizontal and vertical axes </li>
  // <li> Button 2 <i>should</i> translate in x and y directions, but the code needs fixing</li>
  // <li> Button 3 provides a menu of rendering options <li>
  // <li> The wheel <i>should</i> zoom in and out, but the code needs fixing</li>
  // </ul>

  class View3D : public Canvas3D
  {
  public:
    using RealT = float;

    //! Create a 3D viewer
    View3D(int sx, int sy, bool enableLighting = true,
                bool enableTexture = true);

    //! Sets up GL context
    bool GUIInitGL();

    //! Setup widget.
    bool setup(GLWindow &window);

    //! Add object to scene
    //! If ID!=0, the object is assigned this id number, and
    //! can be accessed using it.
    bool add(const std::shared_ptr<DObject3D> &obj, int id = 0);

    //! Make the scene complete.
    //! If more objects are add()ed after this, a new scene will be started
    void GUISceneComplete();

    //! Access current scene. Not thread safe!
    auto &Scene()
    { return scene; }

    //! Automatically adjust view point
    bool GUIAdjustView();

    //! Fit object to view
    bool GUIFit();

    //! Center output.
    bool GUICenter();

    //! reset rotations.
    bool GUIResetRotation();

    //! Rotates the camera
    //bool GUISetCamera();

    //! Refresh display.
    bool GUIRefresh();

    //! Refresh display. (Thread safe postponed refresh)
    bool Refresh();

#if 0
    //! Access to rotation send signal
    Signal1C<Vector<RealT,2>> &SigRotationTx(void) {return m_sRotationTx;}

    //! Access to rotation receive signal
    Signal1C<Vector<RealT,2>> &SigRotationRx(void) {return m_sRotationRx;}
#endif

    //! Enable or disable camera mastering
    bool Master(bool& bMaster) {m_bMaster = bMaster; return true;}

    //! Enable or disable camera slaving
    bool Slave(bool& bSlave) {m_bSlave = bSlave; return true;}

  protected:
    //! Handle button press.
    bool MousePress(MouseEvent const &me);

    //! Handle button release.
    bool MouseRelease(MouseEvent const &me);

    //! Handle mouse move.
    bool MouseMove(MouseEvent const &me);

    //! Handle mouse wheel.
    bool MouseWheel(MouseEvent const &me);

    //! Resets the camera position.
    //void GUIResetCamera();

    //! Handle configure event
    //bool CBConfigureEvent(GdkEvent *event);

    //! Selects the rendering mode for the backmenu
    // Reads value from the appropriate render mode menu item, and updates the other menu options appropriately.
    bool SelectRenderMode(int& iOption);

    //! Enable or disable frontfaces
    bool GUIFrontFaces(bool& bFront)
      { m_bFront = bFront; GUISetCullMode(); return true; }

    //! Enable or disable backfaces
    bool GUIBackFaces(bool& bBack)
      { m_bBack = bBack; GUISetCullMode(); return true; }

    //! Sets the face culling mode based on member variables
    void GUISetCullMode();

    //! Calculate parameters of view on the scene
    void CalcViewParams(bool AutoExtent);

    bool m_bMaster = false; //!< Are we controlling other cameras?
    bool m_bSlave = false;   //!< Are we slaved to other cameras?

#if 0
    Signal1C<Vector<RealT,2>> m_sRotationTx;
    //! Rotation sending signal
    Signal1C<Vector<RealT,2>> m_sRotationRx; 
    //! Rotation receiving signal
#endif

    //! Rotation slaving function
    bool SlaveRotation(Vector<RealT,2>& rotation);
    //! Sends the updated rotation to slave views
    void SendSlaveSignal();

    Vector<RealT,2> m_vRotation = {0,0};  //!< Rotation parameters

    bool sceneComplete = false; //!< Is the scene complete?
    std::shared_ptr<DObjectSet3D> scene; //!< List of current render instructions.
    //MenuC backMenu;
    //Vector<RealT,3> viewObject; // looking at point.
    //bool useRotate;
    //Vector<RealT,3> viewRotate;    // Rotation to apply.
    //RealT fov;
    //bool enablerefresh; // Are we allowed to refresh

    bool initDone = false; //!< Has initalization been completed ?

    //view settings (these are parameters of the camera)
    GLdouble m_sceneExtent = 1.0;
    Vector<RealT,3> m_viewPoint { 0,0,10 };  // Where we are.
    Vector<RealT,3> m_sceneCenter { 0,0,0 }; // looking at point.

    bool m_bTextureStatus = true;
    bool m_bLightingStatus = true;

    // Mouse position storage
    Index<2> m_lastMousePos {0,0};
    bool m_bIsDragging = false;

    // Render mode menu option handles
    //MenuCheckItemC m_oRenderOpts[4];

    // Culling options
    bool m_bFront = false;
    bool m_bBack = false;

    std::shared_mutex viewLock;
    CallbackSet mCallbacks;
  };


}

