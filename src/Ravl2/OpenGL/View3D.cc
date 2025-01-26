// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
///////////////////////////////////////////////////

#include <GL/glu.h>
#include "Ravl2/OpenGL/View3D.hh"

#define DODEBUG 1
#if DODEBUG
#define ONDEBUG(x) x
static std::string GLGetString(GLenum Name)
{
  const char *ptr = reinterpret_cast<const char *>(glGetString(Name));
  std::string res = ptr != NULL ? ptr : "NULL";
  return res;
}
#else
#define ONDEBUG(x)
#endif


namespace Ravl2 {

  //: Default constructor.
  View3D::View3D(int sx,int sy,bool enableLighting,bool enableTexture)
    : Canvas3D(sx,sy),
      m_bTextureStatus(enableTexture),
      m_bLightingStatus(enableLighting)
  {
    ONDEBUG(SPDLOG_INFO("View3DBodyC::View3DBodyC(), Called. "));
  }

  bool View3D::GUIInitGL()
  {
    ONDEBUG(SPDLOG_INFO("View3DBodyC::InitGL(), Called. "));
    // Set up culling
    GUISetCullMode();
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    // Init shade model
    glShadeModel(GL_SMOOTH);

    SetRenderMode(C3D_SMOOTH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

    ONDEBUG(SPDLOG_INFO("OpenGL vendor    : {} ",GLGetString(GL_VENDOR)));
    ONDEBUG(SPDLOG_INFO("OpenGL renderer  : {} ",GLGetString(GL_RENDERER)));
    ONDEBUG(SPDLOG_INFO("OpenGL version   : {} ",GLGetString(GL_VERSION)));
    ONDEBUG(SPDLOG_INFO("OpenGL extensions: {} ",GLGetString(GL_EXTENSIONS)));

    // Let everyone know we're ready to go.
    initDone = true;
    return true;
  }

  //! Setup widget.
  bool View3D::setup(GLWindow &window)
  {
    //mCallbacks += window.mouseButtonCallback();
  }

#if 0
  //: Setup widget.
  bool View3DBodyC::Create(GtkWidget *Parent)
  {
    ONDEBUG(SPDLOG_INFO("View3DBodyC::Create(), Called. "));

    ConnectRef(Signal("button_press_event"),   *this, &View3DBodyC::MousePress);
    ConnectRef(Signal("button_release_event"), *this, &View3DBodyC::MouseRelease);
    ConnectRef(Signal("motion_notify_event"),  *this, &View3DBodyC::MouseMove);
    ConnectRef(Signal("scroll_event"),         *this, &View3DBodyC::MouseWheel);
    ConnectRef(Signal("expose_event"),         *this, &View3DBodyC::Refresh);
    ConnectRef(m_sRotationRx,                  *this, &View3DBodyC::SlaveRotation);

    if(!Canvas3D::Create(Parent))
    {
      // Get this sorted out early.
      SPDLOG_INFO("View3DBodyC::Create(), ERROR: Canvas3D create failed. \n";
      return false;
    }

    ONDEBUG(SPDLOG_INFO("View3DBodyC::Create(), Setting up canvas initialisation. "));

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

    ONDEBUG(SPDLOG_INFO("View3DBodyC::Create(), Doing setup. "));

    SetTextureMode(m_bTextureStatus);
    SetLightingMode(m_bLightingStatus);

    // Put Initialise OpenGL into queue
    Manager.Queue(Trigger(View3DC(*this), &View3DC::GUIInitGL));

    // Setup lights and cameras (it is ok to delayed render here)
    //Put(DLight3DC(RealRGBValueC(1, 1, 1), Point<RealT,3>(0, 0, 10)));
    Manager.Queue(Trigger(View3DC(*this), &View3DC::GUIAdjustView));

    ONDEBUG(SPDLOG_INFO("View3DBodyC::Create(), Done. "));
    return true;
  }
#endif
  
  //: ADD object into the view.
  bool View3D::add(const std::shared_ptr<DObject3D> &obj, int id)
  {
    ONDEBUG(SPDLOG_INFO("View3DBodyC::add(), Called. "));
    (void) id;
    {
      std::lock_guard lockHold(viewLock);
      if(sceneComplete || !scene) {
        scene = std::make_shared<DObjectSet3D>();
        sceneComplete = false;
      }
      scene->GUIAdd(obj);
    }

    ONDEBUG(SPDLOG_INFO("View3DBodyC::add(), Done. "));
    return true;
  }

  //: Make the scene complete.
  // If more objects are add()ed after this, a new scene will be started
  void View3D::GUISceneComplete()
  {
    sceneComplete = true;
    CalcViewParams(true);
    GUIBeginGL();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    m_viewPoint = Vector<RealT,3>(0.0f, 0.0f, 5.0f * float(m_sceneExtent));
    GUIAdjustView();
    GUIRefresh();
  }

  //: adjust view point
  bool View3D::GUIAdjustView()
  {
    ONDEBUG(SPDLOG_INFO("View3DBodyC::AdjustView(), Called. "));

    //lock scene
    std::shared_lock lockHold(viewLock);

    if(!scene) {
      return false;
    }
    
    Vector<RealT,3> lookAt = m_sceneCenter; // Vector<RealT,3>(0,0,0)
    GLdouble dist = GLdouble(euclidDistance(lookAt,m_viewPoint));
    //cerr << "View3DBodyC::GUIAdjustView :" << lookAt << "  dist:" << dist << std::endl;
    if(dist <= 0)
      dist = 0.01;
    //if(dist <= m_sceneExtent)
    //  SPDLOG_INFO("View point could be inside scene\n";

    GUIBeginGL();
    //get viewport parameters
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    //cerr << "View port:" << viewport[0] << "  " << viewport[1] << "  " << viewport[2] << "  " << viewport[3] << "  " << std::endl;
    
    GLdouble fNear = dist - (m_sceneExtent*2);
    GLdouble fFar = dist + (m_sceneExtent*2);
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
    PixelRGB32F val(1., 1., 1.);
    DLight3D(val, m_viewPoint).GUIRender(*this);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //cerr << "ext:" << extHor << "   " << extVer << std::endl;
    //cerr << "depth:" << fNear << "   " << fFar << std::endl;
    glFrustum(-extHor, extHor, -extVer, extVer, fNear, fFar);
    
    //setup view point
    gluLookAt(GLdouble(m_viewPoint[0]),   GLdouble(m_viewPoint[1]),   GLdouble(m_viewPoint.z()),
              GLdouble(lookAt[0]),        GLdouble(lookAt[1]),        GLdouble(lookAt.z()),
              0.,                1.,                0.);
    //FTensor<RealT,2><4, 4> projectionMat;
    //glGetDoublev(GL_PROJECTION_MATRIX, &(projectionMat(0,0)));
    //cerr << "pMat:\n" << projectionMat << std::endl;

    return true;
  }

  //: Fit object to view
  bool View3D::GUIFit()
  {
    ONDEBUG(SPDLOG_INFO("View3DBodyC::GUIFit(), Called. "));
    CalcViewParams(true);
    GUIAdjustView();
    GUIRefresh();
    return true;
  }

  //: Center output.
  bool View3D::GUICenter() {
    ONDEBUG(SPDLOG_INFO("View3DBodyC::GUICenter(), Called. "));
    GUIAdjustView();
    GUIRefresh();
    return true;
  }

  //: Center output.
  bool View3D::GUIResetRotation() {
    ONDEBUG(SPDLOG_INFO("View3DBodyC::GUIResetRotation(), Called. "));
    m_vRotation = Vector<RealT,2>(0,0);
    GUIRefresh();
    SendSlaveSignal();
    return true;
  }

  //: Handle button press.
  bool View3D::MousePress(MouseEvent &me)
  {
    (void) me;
#if 0
    ONDEBUG(SPDLOG_INFO("View3DBodyC::MousePress(), Called. '" << me.HasChanged(0) << " " << me.HasChanged(1) << " " << me.HasChanged(2) <<"' "));
    ONDEBUG(SPDLOG_INFO("View3DBodyC::MousePress(),         '" << me.IsPressed(0) << " " << me.IsPressed(1) << " " << me.IsPressed(2) <<"' "));
    if(me.HasChanged(0))
    {
      //save reference position
      m_lastMousePos = me.At();
      m_bIsDragging = true;
    }
    if(me.HasChanged(2))
    {
      ONDEBUG(SPDLOG_INFO("Show menu. "));
      backMenu.Popup();
    }
#endif

    return true;
  }

  //: Handle button release.
  bool View3D::MouseRelease(MouseEvent &me)
  {
    (void) me;
#if 0
    ONDEBUG(SPDLOG_INFO("View3DBodyC::MouseRelease(), Called. '" << me.HasChanged(0) << " " << me.HasChanged(1) << " " << me.HasChanged(2) <<"' "));
    ONDEBUG(SPDLOG_INFO("View3DBodyC::MouseRelease(),         '" << me.IsPressed(0) << " " << me.IsPressed(1) << " " << me.IsPressed(2) <<"' "));
    if(me.HasChanged(0))
    {
      m_bIsDragging = false;
    }
#endif
    return true;
  }

  //: Handle mouse move.
  bool View3D::MouseMove(MouseEvent &me)
  {
    (void) me;
#if 0
    //ONDEBUG(SPDLOG_INFO("View3DBodyC::MouseMove(), Called. '" << me.HasChanged(0) << " " << me.HasChanged(1) << " " << me.HasChanged(2) <<"' "));
    //ONDEBUG(SPDLOG_INFO("View3DBodyC::MouseMove(),         '" << me.IsPressed(0) << " " << me.IsPressed(1) << " " << me.IsPressed(2) <<"' "));
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
      SPDLOG_INFO("translation\n";

      // Calculate individual translations
      // X & Y in GTK coords; hence also Y is inverted
      //m_fXTranslation += (RealT)change[1] / 100.0;
      //m_fYTranslation -= (RealT)change[0] / 100.0;

      // Update display
      GUIRefresh();
      // Make slaved views move
      SendSlaveSignal();
    }
#endif
    return true;
  }

  //: Sends the updated rotation to slave views
  void View3D::SendSlaveSignal() {
    if (m_bMaster) {
      //m_sRotationTx(m_vRotation);
    }
  }

  //: Handle mouse wheel.
#if 0
  bool View3DBodyC::MouseWheel(GdkEvent *event)
  {
    ONDEBUG(SPDLOG_INFO("View3DBodyC::MouseWheel, Called."));
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
#endif

  //: Rotation slaving function
  bool View3D::SlaveRotation(Vector<RealT,2>& rotation)
  { 
    if (m_bSlave) {
      m_vRotation = rotation;
      GUIRefresh();
    }
    return true;
  }

#if 0
  //: Handle configure event
  bool View3DBodyC::CBConfigureEvent(GdkEvent *event)
  {
    if(GUIBeginGL())
    {
      ONDEBUG(SPDLOG_INFO("Reshape. " << widget->allocation.width << " " << widget->allocation.height << ""));
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
#endif

  //: Refresh display.
  bool View3D::GUIRefresh()
  {
    if(!initDone)
    {
      ONDEBUG(SPDLOG_INFO("View3DBodyC::GUIRefresh(), Called. Returning: {} ",initDone));
      return false;
    }

    ONDEBUG(SPDLOG_INFO("View3DBodyC::GUIRefresh(), Called. {} ",static_cast<void *>(this)));

    GUIBeginGL();

    GUIClearBuffers();
    // Render scene
    {
      std::shared_lock lockHold(viewLock);
      if(scene)
      {
        //shift origin to scene centre
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glTranslatef(m_sceneCenter[0], m_sceneCenter[1], m_sceneCenter[2]);
        glRotatef(m_vRotation[0],1,0,0);
        glRotatef(m_vRotation[1],0,1,0);
        glTranslatef(-m_sceneCenter[0], -m_sceneCenter[1], -m_sceneCenter[2]);
	
        scene->GUIRender(*this);
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
  bool View3D::Refresh()
  {
#if 0
    View3DC my(*this);
    Manager.Queue(Trigger(my, &View3DC::GUIRefresh));
#endif
    return true;
  }

  bool View3D::SelectRenderMode(int& iOption)
  {
    (void) iOption;
#if 0
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
#endif
    return true;
  }

  void View3D::GUISetCullMode()
  {
    ONDEBUG(SPDLOG_INFO("View3DBodyC::SetCullMode(), Called. "));

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

  void View3D::CalcViewParams(bool AutoExtent)
  {
    if(AutoExtent)
      m_sceneExtent = GLdouble(scene->GUIExtent()) * 1.1;
    m_sceneCenter = scene->GUICenter();
    //cerr << "scene extent:" << m_sceneExtent << std::endl;
  }
}
