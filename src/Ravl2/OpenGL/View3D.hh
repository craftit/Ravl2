// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.Graphics.3D"
//! author="James Smith"

#pragma once

#include "Ravl2/OpenGL/Canvas3D.hh"

namespace Ravl2 {

  class MouseEventC;
  class View3DC;

  //! @brief 3D Viewer widget.
  // <p>This class enables you to manipulate a 3D object as you view it, using the mouse.  </p>
  // <ul>
  // <li> Button 1 rotates about horizontal and vertical axes </li>
  // <li> Button 2 <i>should</i> translate in x and y directions, but the code needs fixing</li>
  // <li> Button 3 provides a menu of rendering options <li>
  // <li> The wheel <i>should</i> zoom in and out, but the code needs fixing</li>
  // </ul>

  class View3DBodyC : public Canvas3D
  {
  public:
    using RealT = float;

    //: Create a 3D viewer
    View3DBodyC(int sx, int sy, bool enableLighting = true,
                bool enableTexture = true);
    //: Default constructor.

    bool GUIInitGL(void);
    //: Sets up GL context

    virtual bool Create(GtkWidget *Parent);
    //: Setup widget.

    bool GUIAdd(const DObject3DC &r, IntT id = 0);
    //: Add object to scene (call only from GUI thread)
    // If ID!=0, the object is assigned this id number, and
    // can be accessed using it.

    bool add(const DObject3DC &r, IntT id = 0);
    //: Add object to scene
    // If ID!=0, the object is assigned this id number, and
    // can be accessed using it.
    
    void GUISceneComplete();
    //: Make the scene complete.
    // If more objects are add()ed after this, a new scene will be started

    DObjectSet3DC &Scene()
      { return scene; }
    //: Access current scene. Not thread safe!

    bool GUIAdjustView();
    //:Automatically adjust view point

    bool GUIFit();
    //: Fit object to view

    bool GUICenter();
    //: Center output.

    bool GUIResetRotation();
    //: reset rotations.

    bool GUISetCamera(void);
    //: Rotates the camera

    bool GUIRefresh();
    //: Refresh display.

    bool Refresh();
    //: Refresh display. (Thread safe postponded refresh)

    Signal1C<Vector<RealT,2>> &SigRotationTx(void) {return m_sRotationTx;}
    //: Access to rotation send signal
  
    Signal1C<Vector<RealT,2>> &SigRotationRx(void) {return m_sRotationRx;}
    //: Access to rotation receive signal

    bool Master(bool& bMaster) {m_bMaster = bMaster; return true;}
    //: Enable or disable camera mastering

    bool Slave(bool& bSlave) {m_bSlave = bSlave; return true;}
    //: Enable or disable camera slaving

  protected:
    bool MousePress(MouseEventC &me);
    //: Handle button press.

    bool MouseRelease(MouseEventC &me);
    //: Handle button release.

    bool MouseMove(MouseEventC &me);
    //: Handle mouse move.

    bool MouseWheel(GdkEvent *event);
    //: Handle mouse wheel.

    //void GUIResetCamera();
    //: Resets the camera position.

    bool CBConfigureEvent(GdkEvent *event);
    //: Handle configure event

    bool SelectRenderMode(int& iOption);
    //: Selects the rendering mode for the backmenu
    // Reads value from the appropriate render mode menu item, and updates the other menu options appropriately.

    bool GUIFrontFaces(bool& bFront)
      { m_bFront = bFront; GUISetCullMode(); return true; }
    //: Enable or disable frontfaces

    bool GUIBackFaces(bool& bBack)
      { m_bBack = bBack; GUISetCullMode(); return true; }
    //: Enable or disable backfaces

    void GUISetCullMode();
    //: Sets the face culling mode based on member variables

    void CalcViewParams(bool AutoExtent);
    //: Calculate parameters of view on the scene
 
    bool m_bMaster;
    //: Are we controlling other cameras?
    bool m_bSlave;
    //: Are we slaved to other cameras?
    Signal1C<Vector<RealT,2>> m_sRotationTx; 
    //: Rotation sending signal
    Signal1C<Vector<RealT,2>> m_sRotationRx; 
    //: Rotation receiving signal
    bool SlaveRotation(Vector<RealT,2>& rotation);
    //: Rotation slaving function
    void SendSlaveSignal();
    //: Sends the updated rotation to slave views
    Vector<RealT,2> m_vRotation;
    //: View rotation parameters

    bool sceneComplete;
    DObjectSet3DC scene; // List of current render instructions.
    MenuC backMenu;
    //Vector<RealT,3> viewObject; // looking at point.
    //bool useRotate;
    //Vector<RealT,3> viewRotate;    // Rotation to apply.
    //RealT fov;
    //bool enablerefresh; // Are we allowed to refresh

    bool initDone; // Has initalization been completed ?

    //view settings (these are parameters of the camera)
    GLdouble m_sceneExtent;
    Vector<RealT,3> m_viewPoint;  // Where we are.
    Vector<RealT,3> m_sceneCenter; // looking at point.

    bool m_bTextureStatus;
    bool m_bLightingStatus;

    // Mouse position storage
    Index<2> m_lastMousePos;
    bool m_bIsDragging;

    // Render mode menu option handles
    MenuCheckItemC m_oRenderOpts[4];

    // Culling options
    bool m_bFront;
    bool m_bBack;

    RWLockC viewLock;
  };


}

