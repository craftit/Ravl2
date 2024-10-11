// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
///////////////////////////////////////////////////
#include "Ravl2/OpenGL//View3D.hh"

#if 0
#include "Ravl2/GUI/Manager.hh"
#include "Ravl2/GUI/Util.hh"
#include "Ravl2/GUI/Menu.hh"
#include "Ravl2/GUI/MenuCheck.hh"
#include "Ravl2/GUI/MouseEvent.hh"
#include "Ravl2/StdMath.hh"
#include "Ravl2/StdConst.hh"
#include "Ravl2/AxisAngle.hh"
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <GL/glu.h>
#endif

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
static RavlN::std::string GLGetString(GLenum Name)
{
  char *ptr = (char *)glGetString(Name);
  RavlN::std::string res = ptr != NULL ? ptr : "NULL";
  return res;
}
#else
#define ONDEBUG(x)
#endif


namespace Ravl2 {

  //: Default constructor.
  View3DBodyC::View3DBodyC(int sx,int sy,bool enableLighting,bool enableTexture)
    : Canvas3D(sx,sy),
      m_bMaster(false),
      m_bSlave(false),
      m_sRotationTx(Vector<RealT,2>()),
      m_sRotationRx(Vector<RealT,2>()),
      m_vRotation(0,0),
      sceneComplete(false),
      initDone(false),
      m_sceneExtent(1),
      m_viewPoint(0, 0, 10),
      m_sceneCenter(0, 0, 0),
      m_bTextureStatus(enableTexture),
      m_bLightingStatus(enableLighting),
      m_bIsDragging(false),
      m_bFront(true),
      m_bBack(false)
  {
    ONDEBUG(std::cerr << "View3DBodyC::View3DBodyC(), Called. \n");
  }

  bool View3DBodyC::GUIInitGL()
  {
    ONDEBUG(std::cerr << "View3DBodyC::InitGL(), Called. \n");
    // Set up culling
    GUISetCullMode();
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    // Init shade model
    glShadeModel(GL_SMOOTH);
    Canvas3DRenderMode mode = C3D_SMOOTH;
    SetRenderMode(mode);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

    ONDEBUG(std::cerr << "OpenGL vendor    :" << GLGetString(GL_VENDOR) << endl);
    ONDEBUG(std::cerr << "OpenGL renderer  :" << GLGetString(GL_RENDERER) << endl);
    ONDEBUG(std::cerr << "OpenGL version   :" << GLGetString(GL_VERSION) << endl);
    ONDEBUG(std::cerr << "OpenGL extensions:" << GLGetString(GL_EXTENSIONS) << endl);

    // Let everyone know we're ready to go.
    initDone = true;
    return true;
  }

  //: Setup widget.
  bool View3DBodyC::Create(GtkWidget *Parent)
  {
    ONDEBUG(std::cerr << "View3DBodyC::Create(), Called. \n");

    ConnectRef(Signal("button_press_event"),   *this, &View3DBodyC::MousePress);
    ConnectRef(Signal("button_release_event"), *this, &View3DBodyC::MouseRelease);
    ConnectRef(Signal("motion_notify_event"),  *this, &View3DBodyC::MouseMove);
    ConnectRef(Signal("scroll_event"),         *this, &View3DBodyC::MouseWheel);
    ConnectRef(Signal("expose_event"),         *this, &View3DBodyC::Refresh);
    ConnectRef(m_sRotationRx,                  *this, &View3DBodyC::SlaveRotation);

    if(!Canvas3D::Create(Parent))
    {
      // Get this sorted out early.
      std::cerr << "View3DBodyC::Create(), ERROR: Canvas3D create failed. \n";
      return false;
    }

    ONDEBUG(std::cerr << "View3DBodyC::Create(), Setting up canvas initialisation. \n");

    // Setup render options
    m_oRenderOpts[0] = MenuCheckItemC("Points", false);
    m_oRenderOpts[1] = MenuCheckItemC("Wire",   false);
    m_oRenderOpts[2] = MenuCheckItemC("Flat",   false);
    m_oRenderOpts[3] = MenuCheckItemC("Smooth", true);
    for(int i=0; i<4; i++) {
      ConnectRef(m_oRenderOpts[i].SigSelected(), *this, &View3DBodyC::SelectRenderMode, i);
    }

    MenuC renderMenu("Render",
                     m_oRenderOpts[0] +
                     m_oRenderOpts[1] +
                     m_oRenderOpts[2] +
                     m_oRenderOpts[3] +
                     MenuItemSeparator() +
                     MenuCheckItem("Texturing", m_bTextureStatus,  Canvas3DC(*this), &Canvas3DC::SetTextureMode) +
                     MenuCheckItem("Lighting",  m_bLightingStatus, Canvas3DC(*this), &Canvas3DC::SetLightingMode)
                    );

    MenuC facesMenu("Faces",
                    MenuCheckItemR("Front", m_bFront, *this, &View3DBodyC::GUIFrontFaces) +
                    MenuCheckItemR("Back",  m_bBack,  *this, &View3DBodyC::GUIBackFaces)
                   );

    backMenu = MenuC("back",
                     MenuItemR("Center",           *this, &View3DBodyC::GUICenter) +
                     MenuItemR("Fit",              *this, &View3DBodyC::GUIFit) +
                     MenuItemR("Upright",          *this, &View3DBodyC::GUIResetRotation) +
                     //MenuCheckItemR("Auto Center", *this, &View3DBodyC::GUIAutoCenter) +
                     //MenuCheckItemR("Auto Fit",    *this, &View3DBodyC::GUIAutoFit) +
                     MenuItemSeparator() +		      
		                 MenuCheckItemR("Master",m_bMaster,*this,&View3DBodyC::Master) +
		                 MenuCheckItemR("Slave",m_bSlave,*this,&View3DBodyC::Slave) +
                     MenuItemSeparator() +
                     renderMenu +
                     facesMenu
                    );

    ONDEBUG(std::cerr << "View3DBodyC::Create(), Doing setup. \n");

    SetTextureMode(m_bTextureStatus);
    SetLightingMode(m_bLightingStatus);

    // Put Initialise OpenGL into queue
    Manager.Queue(Trigger(View3DC(*this), &View3DC::GUIInitGL));

    // Setup lights and cameras (it is ok to delayed render here)
    //Put(DLight3DC(RealRGBValueC(1, 1, 1), Point<RealT,3>(0, 0, 10)));
    Manager.Queue(Trigger(View3DC(*this), &View3DC::GUIAdjustView));

    ONDEBUG(std::cerr << "View3DBodyC::Create(), Done. \n");
    return true;
  }

  //: ADD object into the view.
  bool View3DBodyC::GUIAdd(const DObject3DC &r, IntT id)
  {
    ONDEBUG(std::cerr << "View3DBodyC::GUIAdd(), Called. \n");
    {
      RWLockHoldC lockHold(viewLock, RWLOCK_WRITE);
      if(sceneComplete || !scene.IsValid()) {
        scene = DObjectSet3DC(true);
        sceneComplete = false;
      }
      if(r.IsValid())
        scene += r;
    }

    ONDEBUG(std::cerr << "View3DBodyC::GUIAdd(), Done. \n");
    return true;
  }

  //: ADD object into the view.
  bool View3DBodyC::add(const DObject3DC &r, IntT id)
  {
    ONDEBUG(std::cerr << "View3DBodyC::add(), Called. \n");
    {
      RWLockHoldC lockHold(viewLock, RWLOCK_WRITE);
      if(sceneComplete || !scene.IsValid()) {
        scene = DObjectSet3DC(true);
        sceneComplete = false;
      }
      if(r.IsValid())
        scene += r;
    }

    ONDEBUG(std::cerr << "View3DBodyC::add(), Done. \n");
    return true;
  }

  //: Make the scene complete.
  // If more objects are add()ed after this, a new scene will be started
  void View3DBodyC::GUISceneComplete()
  {
    sceneComplete = true;
    CalcViewParams(true);
    GUIBeginGL();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    m_viewPoint = Vector<RealT,3>(0., 0., 5. * m_sceneExtent);
    GUIAdjustView();
    GUIRefresh();
  }

  //: adjust view point
  bool View3DBodyC::GUIAdjustView()
  {
    ONDEBUG(std::cerr << "View3DBodyC::AdjustView(), Called. \n");

    //lock scene
    RWLockHoldC lockHold(viewLock, RWLOCK_READONLY);

    if(!scene.IsValid())
      return false;
    
    Vector<RealT,3> lookAt = m_sceneCenter; // Vector<RealT,3>(0,0,0)
    RealT dist = lookAt.EuclidDistance(m_viewPoint);
    //cerr << "View3DBodyC::GUIAdjustView :" << lookAt << "  dist:" << dist << std::endl;
    if(dist <= 0)
      dist = 0.01;
    //if(dist <= m_sceneExtent)
    //  std::cerr << "View point could be inside scene\n";

    GUIBeginGL();
    //get viewport parameters
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    //cerr << "View port:" << viewport[0] << "  " << viewport[1] << "  " << viewport[2] << "  " << viewport[3] << "  " << std::endl;
    
    GLdouble fNear = dist - m_sceneExtent*2;
    GLdouble fFar = dist + m_sceneExtent*2;
    if(fNear < 0.1)
      fNear = 0.1;
    GLdouble extent = m_sceneExtent * fNear / dist;

    GLdouble extHor, extVer;
    if(viewport[2] < viewport[3])
    {
      extHor = extent;
      extVer = extent * viewport[3] / viewport[2];
    }
    else
    {
      extHor = extent * viewport[2] / viewport[3];
      extVer = extent;
    }

    //setup light
    Canvas3DC me(*this);
    RealRGBValueC val(1., 1., 1.);
    DLight3DC(val, m_viewPoint).GUIRender(me);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //cerr << "ext:" << extHor << "   " << extVer << std::endl;
    //cerr << "depth:" << fNear << "   " << fFar << std::endl;
    glFrustum(-extHor, extHor, -extVer, extVer, fNear, fFar);
    
    //setup view point
    gluLookAt(m_viewPoint[0],   m_viewPoint[1],   m_viewPoint.Z(),
              lookAt[0],        lookAt[1],        lookAt.Z(),
              0.,                1.,                0.);
    //FTensor<RealT,2><4, 4> projectionMat;
    //glGetDoublev(GL_PROJECTION_MATRIX, &(projectionMat(0,0)));
    //cerr << "pMat:\n" << projectionMat << std::endl;


    return true;
  }

  //: Fit object to view
  bool View3DBodyC::GUIFit()
  {
    ONDEBUG(std::cerr << "View3DBodyC::GUIFit(), Called. \n");
    CalcViewParams(true);
    GUIAdjustView();
    GUIRefresh();
    return true;
  }

  //: Center output.
  bool View3DBodyC::GUICenter() {
    ONDEBUG(std::cerr << "View3DBodyC::GUICenter(), Called. \n");
    GUIAdjustView();
    GUIRefresh();
    return true;
  }

  //: Center output.
  bool View3DBodyC::GUIResetRotation() {
    ONDEBUG(std::cerr << "View3DBodyC::GUIResetRotation(), Called. \n");
    m_vRotation = Vector<RealT,2>(0,0);
    GUIRefresh();
    SendSlaveSignal();
    return true;
  }

  //: Handle button press.
  bool View3DBodyC::MousePress(MouseEventC &me)
  {
    ONDEBUG(std::cerr << "View3DBodyC::MousePress(), Called. '" << me.HasChanged(0) << " " << me.HasChanged(1) << " " << me.HasChanged(2) <<"' \n");
    ONDEBUG(std::cerr << "View3DBodyC::MousePress(),         '" << me.IsPressed(0) << " " << me.IsPressed(1) << " " << me.IsPressed(2) <<"' \n");

    if(me.HasChanged(0))
    {
      //save reference position
      m_lastMousePos = me.At();
      m_bIsDragging = true;
    }
    if(me.HasChanged(2))
    {
      ONDEBUG(std::cerr << "Show menu. \n");
      backMenu.Popup();
    }

    return true;
  }

  //: Handle button release.
  bool View3DBodyC::MouseRelease(MouseEventC &me)
  {
    ONDEBUG(std::cerr << "View3DBodyC::MouseRelease(), Called. '" << me.HasChanged(0) << " " << me.HasChanged(1) << " " << me.HasChanged(2) <<"' \n");
    ONDEBUG(std::cerr << "View3DBodyC::MouseRelease(),         '" << me.IsPressed(0) << " " << me.IsPressed(1) << " " << me.IsPressed(2) <<"' \n");
    if(me.HasChanged(0))
    {
      m_bIsDragging = false;
    }
    return true;
  }

  //: Handle mouse move.
  bool View3DBodyC::MouseMove(MouseEventC &me) {
    //ONDEBUG(std::cerr << "View3DBodyC::MouseMove(), Called. '" << me.HasChanged(0) << " " << me.HasChanged(1) << " " << me.HasChanged(2) <<"' \n");
    //ONDEBUG(std::cerr << "View3DBodyC::MouseMove(),         '" << me.IsPressed(0) << " " << me.IsPressed(1) << " " << me.IsPressed(2) <<"' \n");
    //cerr << "View3DBodyC::MouseMove(), Called. \n";

    // Calculate change
    Index<2> change = me.At() - m_lastMousePos;
    m_lastMousePos = me.At();
    //cerr << "change:" << change << std::endl;

    // Rotate when button 0 pressed
    if(me.IsPressed(0) && m_bIsDragging)
    {
      //cerr << "rotation\n";

      if(change[0] == 0 && change[1] == 0)
        return true;
      m_vRotation[0] += change[0];
      m_vRotation[1] += change[1];
      if (m_vRotation[0] > 90) m_vRotation[0] = 90;
      if (m_vRotation[0] < -90) m_vRotation[0] = -90;
      GUIRefresh();
      // Make slaved views move
      SendSlaveSignal();

    }

    // Translate when button 1 pressed
    else if (me.IsPressed(1) && m_bIsDragging) {
      std::cerr << "translation\n";

      // Calculate individual translations
      // X & Y in GTK coords; hence also Y is inverted
      //m_fXTranslation += (RealT)change[1] / 100.0;
      //m_fYTranslation -= (RealT)change[0] / 100.0;

      // Update display
      GUIRefresh();
      // Make slaved views move
      SendSlaveSignal();
    }

    return true;
  }

  //: Sends the updated rotation to slave views
  void View3DBodyC::SendSlaveSignal() {
    if (m_bMaster) {
      m_sRotationTx(m_vRotation);
    }
  }

  //: Handle mouse wheel.
  bool View3DBodyC::MouseWheel(GdkEvent *event)
  {
    ONDEBUG(std::cerr << "View3DBodyC::MouseWheel, Called.\n");
    GdkEventScroll &scrollEvent = (GdkEventScroll &) *event;
    bool shiftKey = (scrollEvent.state & GDK_SHIFT_MASK) != 0;
    //bool ctrlKey  = (scrollEvent.state & GDK_CONTROL_MASK) != 0;
    //cerr << "shift:" << shiftKey << "   ctrl:" << ctrlKey << std::endl;

    switch(scrollEvent.direction)
    {
    case GDK_SCROLL_UP:
    case GDK_SCROLL_LEFT:
      //cerr << "up\n";
      if(!shiftKey)
        m_sceneExtent *= 1.2; //change zoom
      else
        m_viewPoint = m_viewPoint / 1.2;
      GUIAdjustView();
      GUIRefresh();
      break;
    case GDK_SCROLL_DOWN:
    case GDK_SCROLL_RIGHT:
      //cerr << "down\n";
      if(!shiftKey)
        m_sceneExtent /= 1.2; //change zoom
      else
        m_viewPoint = m_viewPoint * 1.2;
      GUIAdjustView();
      GUIRefresh();
      break;
    }
    //cout << "vp:" << m_viewPoint << std::endl;

    return true;
  }

  //: Rotation slaving function
  bool View3DBodyC::SlaveRotation(Vector<RealT,2>& rotation)
  { 
    if (m_bSlave) {
      m_vRotation = rotation;
      GUIRefresh();
    }
    return true;
  }

  //: Handle configure event
  bool View3DBodyC::CBConfigureEvent(GdkEvent *event)
  {
    if(GUIBeginGL())
    {
      ONDEBUG(std::cerr << "Reshape. " << widget->allocation.width << " " << widget->allocation.height << "\n");
      glViewport(0, 0, widget->allocation.width, widget->allocation.height);

      //CalcViewParams(m_bAutoFit);
      GUIAdjustView();
      //FTensor<RealT,2><4, 4> projectionMat;
      //glGetDoublev(GL_PROJECTION_MATRIX, &(projectionMat(0,0)));
      //cerr << "pMat:\n" << projectionMat << std::endl;
    }
    GUIEndGL();
    return true;
  }

  //: Refresh display.
  bool View3DBodyC::GUIRefresh()
  {
    if(!initDone)
    {
      ONDEBUG(std::cerr << "View3DBodyC::GUIRefresh(), Called. Returning" << initDone << "\n");
      return false;
    }

    ONDEBUG(std::cerr << "View3DBodyC::GUIRefresh(), Called. " << ((void *) widget) << "\n");

    GUIBeginGL();

    GUIClearBuffers();
    // Render scene
    {
      RWLockHoldC lockHold(viewLock, RWLOCK_READONLY);
      if(scene.IsValid())
      {
        //shift origin to scene centre
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glTranslated(m_sceneCenter[0], m_sceneCenter[1], m_sceneCenter.Z());
        glRotated(m_vRotation[0],1,0,0);
        glRotated(m_vRotation[1],0,1,0);
        glTranslated(-m_sceneCenter[0], -m_sceneCenter[1], -m_sceneCenter.Z());
	
        Canvas3DC my(*this);
        scene.GUIRender(my);
        glPopMatrix();
      }
    }

    // show scene
    GUISwapBuffers();   

    // Finished
    GUIEndGL();
    return true;
  }

  //: Refresh display. (Thread safe postponded refresh)
  bool View3DBodyC::Refresh()
  {
    View3DC my(*this);
    Manager.Queue(Trigger(my, &View3DC::GUIRefresh));
    return true;
  }

  bool View3DBodyC::SelectRenderMode(int& iOption) {
    bool bVal = m_oRenderOpts[iOption].IsActive();
    if (bVal) {
      for (int i=0; i<4; i++) {
        if (i!=iOption) {
          m_oRenderOpts[i].SetActive(false);
        }
      }
      Canvas3DRenderMode mode;
      switch (iOption) {
      case 0:
        mode = C3D_POINT;
        break;
      case 1:
        mode = C3D_WIRE;
        break;
      case 2:
        mode = C3D_FLAT;
        break;
      case 3:
      default:
        mode = C3D_SMOOTH;
        break;
      }
      SetRenderMode(mode);
    }
    else {
      int iNumTrue = 0;
      for (int i=0; i<4; i++) {
        if (i!=iOption && m_oRenderOpts[i].IsActive()) iNumTrue++;
      }
      if (iNumTrue == 0) {
        m_oRenderOpts[iOption].SetActive(true);
      }
    }
    return true;
  }

  void View3DBodyC::GUISetCullMode()
  {
    ONDEBUG(std::cerr << "View3DBodyC::SetCullMode(), Called. \n");

    GUIBeginGL();
    if(m_bFront) {
      if (m_bBack) {
        glDisable(GL_CULL_FACE);
      }
      else {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
      }
    }
    else {
      if (m_bBack) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
      }
      else {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT_AND_BACK);
      }
    }
    //cerr << "Culling:" << (glIsEnabled(GL_CULL_FACE) == GL_TRUE ? "enabled" : "disabled") << std::endl;
    //GLint cullMode;
    //glGetIntegerv(GL_CULL_FACE_MODE, &cullMode);
    //cerr << "mode:" << cullMode << std::endl;
  }

  void View3DBodyC::CalcViewParams(bool AutoExtent)
  {
    if(AutoExtent)
      m_sceneExtent = scene.GUIExtent() * 1.1;
    m_sceneCenter = scene.GUICenter();
    //cerr << "scene extent:" << m_sceneExtent << std::endl;
  }
}
