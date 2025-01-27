// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
///////////////////////////////////////////////////

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GL/glu.h>
#include "Ravl2/OpenGL/View3D.hh"
#include "Ravl2/OpenGL/GLWindow.hh"
#include "Ravl2/OpenGL/MouseEvent.hh"

#define DODEBUG 1
#if DODEBUG
#define ONDEBUG(x) x
static std::string GLGetString(GLenum Name)
{
  const char *ptr = reinterpret_cast<const char *>(glGetString(Name));
  std::string res = ptr != nullptr ? ptr : "NULL";
  return res;
}
#else
#define ONDEBUG(x)
#endif


namespace Ravl2 {

  //! Convert ravl vector to glm vector.
  template <typename RealT>
  [[nodiscard]] glm::vec<3, RealT> toGLM(const Vector<RealT, 3> &vec)
  {
    return glm::vec<3, RealT>(vec(0), vec(1), vec(2));
  }

  //! Convert ravl point to glm vector.
  template <typename RealT>
  [[nodiscard]] glm::vec<2, RealT> toGLM(const Point<RealT, 2> &point)
  {
    return glm::vec<2, RealT>(point(0), point(1));
  }


  //: Default constructor.
  View3D::View3D(int sx_,int sy_,bool enableLighting,bool enableTexture)
    : Canvas3D(sx_,sy_),
      m_bTextureStatus(enableTexture),
      m_bLightingStatus(enableLighting)
  {
    ONDEBUG(SPDLOG_INFO("View3D::View3D(), Called. "));
  }

  bool View3D::GUIInitGL()
  {
    ONDEBUG(SPDLOG_INFO("View3D::InitGL(), Called. "));

#if 0
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
#elif 0
    /* Enable a single OpenGL light. */
    static GLfloat light_diffuse[] = {1.0, 0.0, 0.0, 1.0};  /* Red diffuse light. */
    static GLfloat light_position[] = {1.0, 1.0, 1.0, 0.0};  /* Infinite light location. */

    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    /* Use depth buffering for hidden surface elimination. */
    glEnable(GL_DEPTH_TEST);

    /* Setup the view of the cube. */
    glMatrixMode(GL_PROJECTION);
    gluPerspective( /* field of view in degree */ 40.0,
                   /* aspect ratio */ 1.0,
                   /* Z near */ 1.0, /* Z far */ 10.0);
    glMatrixMode(GL_MODELVIEW);
    gluLookAt(0.0, 0.0, 5.0,  /* eye is at (0,0,5) */
              0.0, 0.0, 0.0,      /* center is at (0,0,0) */
              0.0, 1.0, 0.);      /* up is in positive Y direction */

    /* Adjust cube position to be aesthetic angle. */
    glTranslatef(0.0, 0.0, -1.0);
    glRotatef(60, 1.0, 0.0, 0.0);
    glRotatef(-20, 0.0, 0.0, 1.0);

#else
    /* Use depth buffering for hidden surface elimination. */
    glEnable(GL_DEPTH_TEST);

    // calculate ViewProjection matrix
    float aspectRatio = 1.0f;
    if(mViewSize[0] != 0 && mViewSize[1] != 0) {
      aspectRatio = float(mViewSize[1]) / float(mViewSize[0]);
    }

    mMatProjection = glm::perspective(90.0f, aspectRatio, 0.1f, 100.f);

    // translate the world/view position
    mMatModelView = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f));

    // Compose things.
    mMatProjectionView = mMatProjection * mMatModelView;

#endif

    //glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

    ONDEBUG(SPDLOG_INFO("OpenGL vendor    : {} ",GLGetString(GL_VENDOR)));
    ONDEBUG(SPDLOG_INFO("OpenGL renderer  : {} ",GLGetString(GL_RENDERER)));
    ONDEBUG(SPDLOG_INFO("OpenGL version   : {} ",GLGetString(GL_VERSION)));
    ONDEBUG(SPDLOG_INFO("OpenGL extensions: {} ",GLGetString(GL_EXTENSIONS)));

    // Let everyone know we're ready to go.
    initDone = true;
    return true;
  }

  //!  Handle configure event
  bool View3D::CBConfigureEvent(int width, int height)
  {
    mViewSize = {height, width};
    GUIAdjustView();
    return true;
  }


  //! Setup widget.
  bool View3D::setup(GLWindow &window)
  {
    //mCallbacks += window.mouseButtonCallback();
    mCallbacks += window.addMouseEventCallback([&](const MouseEvent &event) {
      switch(event.eventType())
      {
        case MouseEventTypeT::MouseMove:
          MouseMove(event);
          break;
        case MouseEventTypeT::MousePress:
          MousePress(event);
          break;
        case MouseEventTypeT::MouseRelease:
          MouseRelease(event);
          break;
      }
    });

    mCallbacks += window.addScrollCallback([&](double xOffset, double yOffset) {
      scroll(xOffset, yOffset);
    });
    return true;
  }


#if 0
  //: Setup widget.
  bool View3D::Create(GtkWidget *Parent)
  {
    ONDEBUG(SPDLOG_INFO("View3D::Create(), Called. "));

    ConnectRef(Signal("button_press_event"),   *this, &View3D::MousePress);
    ConnectRef(Signal("button_release_event"), *this, &View3D::MouseRelease);
    ConnectRef(Signal("motion_notify_event"),  *this, &View3D::MouseMove);
    ConnectRef(Signal("scroll_event"),         *this, &View3D::MouseWheel);
    ConnectRef(Signal("expose_event"),         *this, &View3D::Refresh);
    ConnectRef(m_sRotationRx,                  *this, &View3D::SlaveRotation);

    if(!Canvas3D::Create(Parent))
    {
      // Get this sorted out early.
      SPDLOG_INFO("View3D::Create(), ERROR: Canvas3D create failed. \n";
      return false;
    }

    ONDEBUG(SPDLOG_INFO("View3D::Create(), Setting up canvas initialisation. "));

    // Setup render options
    m_oRenderOpts[0] = MenuCheckItemC("Points", false);
    m_oRenderOpts[1] = MenuCheckItemC("Wire",   false);
    m_oRenderOpts[2] = MenuCheckItemC("Flat",   false);
    m_oRenderOpts[3] = MenuCheckItemC("Smooth", true);
    for(int i=0; i<4; i++) {
      ConnectRef(m_oRenderOpts[i].SigSelected(), *this, &View3D::SelectRenderMode, i);
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
                    MenuCheckItemR("Front", m_bFront, *this, &View3D::GUIFrontFaces) +
                    MenuCheckItemR("Back",  m_bBack,  *this, &View3D::GUIBackFaces)
                   );

    backMenu = MenuC("back",
                     MenuItemR("Center",           *this, &View3D::GUICenter) +
                     MenuItemR("Fit",              *this, &View3D::GUIFit) +
                     MenuItemR("Upright",          *this, &View3D::GUIResetRotation) +
                     //MenuCheckItemR("Auto Center", *this, &View3D::GUIAutoCenter) +
                     //MenuCheckItemR("Auto Fit",    *this, &View3D::GUIAutoFit) +
                     MenuItemSeparator() +		      
		                 MenuCheckItemR("Master",m_bMaster,*this,&View3D::Master) +
		                 MenuCheckItemR("Slave",m_bSlave,*this,&View3D::Slave) +
                     MenuItemSeparator() +
                     renderMenu +
                     facesMenu
                    );

    ONDEBUG(SPDLOG_INFO("View3D::Create(), Doing setup. "));

    SetTextureMode(m_bTextureStatus);
    SetLightingMode(m_bLightingStatus);

    // Put Initialise OpenGL into queue
    Manager.Queue(Trigger(View3DC(*this), &View3DC::GUIInitGL));

    // Setup lights and cameras (it is ok to delayed render here)
    //Put(DLight3DC(RealRGBValueC(1, 1, 1), Point<RealT,3>(0, 0, 10)));
    Manager.Queue(Trigger(View3DC(*this), &View3DC::GUIAdjustView));

    ONDEBUG(SPDLOG_INFO("View3D::Create(), Done. "));
    return true;
  }
#endif
  
  //: ADD object into the view.
  bool View3D::add(const std::shared_ptr<DObject3D> &obj, int id)
  {
    ONDEBUG(SPDLOG_INFO("View3D::add(), Called. "));
    (void) id;
    obj->GUIInit(*this);
    {
      std::lock_guard lockHold(viewLock);
      if(sceneComplete || !scene) {
        scene = std::make_shared<DObjectSet3D>();
        sceneComplete = false;
      }
      scene->GUIAdd(obj);
    }

    ONDEBUG(SPDLOG_INFO("View3D::add(), Done. "));
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
    ONDEBUG(SPDLOG_INFO("View3D::AdjustView(), Called. "));
#if 1
    // calculate ViewProjection matrix
    float aspectRatio = 1.0f;
    if(mViewSize[0] != 0 && mViewSize[1] != 0) {
      aspectRatio = float(mViewSize[1]) / float(mViewSize[0]);
    }

    mMatProjection = glm::perspective(90.0f, aspectRatio, 0.1f, 100.f);

    // translate the world/view position
    mMatModelView = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f));

    // Compose things.
    mMatProjectionView = mMatProjection * mMatModelView;

#else
    //lock scene
    std::shared_lock lockHold(viewLock);

    if(!scene) {
      return false;
    }
    
    Vector<RealT,3> lookAt = m_sceneCenter; // Vector<RealT,3>(0,0,0)
    GLdouble dist = GLdouble(euclidDistance(lookAt,m_viewPoint));
    //cerr << "View3D::GUIAdjustView :" << lookAt << "  dist:" << dist << std::endl;
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
    gluLookAt(GLdouble(m_viewPoint[0]),   GLdouble(m_viewPoint[1]),   GLdouble(m_viewPoint[2]),
              GLdouble(lookAt[0]),        GLdouble(lookAt[1]),        GLdouble(lookAt[2]),
              0.,                1.,                0.);
    //FTensor<RealT,2><4, 4> projectionMat;
    //glGetDoublev(GL_PROJECTION_MATRIX, &(projectionMat(0,0)));
    //cerr << "pMat:\n" << projectionMat << std::endl;
#endif
    return true;
  }

  //: Fit object to view
  bool View3D::GUIFit()
  {
    ONDEBUG(SPDLOG_INFO("View3D::GUIFit(), Called. "));
    CalcViewParams(true);
    GUIAdjustView();
    GUIRefresh();
    return true;
  }

  //: Center output.
  bool View3D::GUICenter() {
    ONDEBUG(SPDLOG_INFO("View3D::GUICenter(), Called. "));
    GUIAdjustView();
    GUIRefresh();
    return true;
  }

  //: Center output.
  bool View3D::GUIResetRotation() {
    ONDEBUG(SPDLOG_INFO("View3D::GUIResetRotation(), Called. "));
    m_vRotation = Vector<RealT,2>(0,0);
    GUIRefresh();
    SendSlaveSignal();
    return true;
  }

  //: Handle button press.
  bool View3D::MousePress(MouseEvent const &me)
  {
    ONDEBUG(SPDLOG_INFO("View3D::MousePress(), Called. {}  {}  {} ",me.HasChanged(0), me.HasChanged(1), me.HasChanged(2)));
    ONDEBUG(SPDLOG_INFO("View3D::MousePress(),         {}  {}  {} ",me.IsPressed(0), me.IsPressed(1), me.IsPressed(2)));
    if(me.HasChanged(0))
    {
      //save reference position
      m_lastMousePos = me.At();
      m_bIsDragging = true;
    }
    if(me.HasChanged(2))
    {
      ONDEBUG(SPDLOG_INFO("Show menu. "));
      //backMenu.Popup();
    }
    return true;
  }

  //: Handle button release.
  bool View3D::MouseRelease(MouseEvent const &me)
  {
    ONDEBUG(SPDLOG_INFO("View3D::MouseRelease(), Called.  {} {} {} ", me.HasChanged(0), me.HasChanged(1), me.HasChanged(2)));
    ONDEBUG(SPDLOG_INFO("View3D::MouseRelease(),          {} {} {} ", me.IsPressed(0), me.IsPressed(1), me.IsPressed(2)));
    if(me.HasChanged(0))
    {
      m_bIsDragging = false;
    }
    return true;
  }

  //: Handle mouse move.
  bool View3D::MouseMove(MouseEvent const &me)
  {
    //ONDEBUG(SPDLOG_INFO("View3D::MouseMove(), Called. '", me.HasChanged(0), me.HasChanged(1), me.HasChanged(2) <<"' "));
    //ONDEBUG(SPDLOG_INFO("View3D::MouseMove(),         '", me.IsPressed(0), me.IsPressed(1), me.IsPressed(2) <<"' "));

    // Calculate change
    Index<2> change = me.At() - m_lastMousePos;
    m_lastMousePos = me.At();

    // Rotate when button 0 pressed
    if(me.IsPressed(0) && m_bIsDragging)
    {
      //SPDLOG_INFO("Rotation");
      if(change[0] == 0 && change[1] == 0)
        return true;
      m_vRotation[0] += float(change[0]);
      m_vRotation[1] += float(change[1]);
      if (m_vRotation[0] > 90) m_vRotation[0] = 90;
      if (m_vRotation[0] < -90) m_vRotation[0] = -90;
      //GUIRefresh();
      // Make slaved views move
      SendSlaveSignal();
    }

    // Translate when button 1 pressed
    else if (me.IsPressed(1) && m_bIsDragging) {
      //SPDLOG_INFO("translation");

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
  void View3D::SendSlaveSignal() {
    if (m_bMaster) {
      //m_sRotationTx(m_vRotation);
    }
  }

  //: Handle mouse wheel.
  void View3D::scroll(double xOffset, double yOffset)
  {
    (void) xOffset;
    ONDEBUG(SPDLOG_INFO("View3D::MouseWheel, Called."));

    if(isNearZero(yOffset))
      return;
    if(yOffset > 0) {
      m_viewPoint = m_viewPoint / 1.2;
    } else {
      m_viewPoint = m_viewPoint * 1.2;
    }
    GUIAdjustView();

#if 0
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
#endif
  }

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
  bool View3D::CBConfigureEvent(GdkEvent *event)
  {
    if(GUIBeginGL())
    {
      ONDEBUG(SPDLOG_INFO("Reshape. {} {}  ",widget->allocation.width, widget->allocation.height));
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
      ONDEBUG(SPDLOG_INFO("View3D::GUIRefresh(), Called. Returning: {} ",initDone));
      return false;
    }

    //ONDEBUG(SPDLOG_INFO("View3D::GUIRefresh(), Called. {} ",static_cast<void *>(this)));

    GUIBeginGL();

    GUIClearBuffers();
#if 1

    auto view = mMatModelView;

    view = glm::translate(view, -toGLM(m_sceneCenter));
    view = glm::rotate(view, deg2rad(m_vRotation[0]), glm::vec3(1.0f, 0.0f, 0.0f));
    view = glm::rotate(view, deg2rad(m_vRotation[1]), glm::vec3(0.0f, 1.0f, 0.0f));
    view = glm::translate(view, toGLM(m_sceneCenter));

    // Compose things.
    mMatProjectionView = mMatProjection * view;

    scene->GUIRender(*this);
#else
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
#endif

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
    ONDEBUG(SPDLOG_INFO("View3D::SetCullMode(), Called. "));

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
