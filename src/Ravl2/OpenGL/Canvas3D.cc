// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
/////////////////////////////////////////////////////

#include <string>
#include <GL/gl.h>

#include "Ravl2/OpenGL/Canvas3D.hh"
#include "Ravl2/Array.hh"
//#include "Ravl2/Image/Reflect.hh"
//#include "Ravl2/Image/ByteRGBValue.hh"
//#include "Ravl2/Threads/SemaphoreRC.hh"
//#include "Ravl2/CallMethodRefs.hh"

#include "Ravl2/OpenGL/GLContext.hh"


#if RAVL_USE_GTKGLEXT
#include <GL/glu.h>
#endif

#ifndef VISUAL_CPP
#include <GL/glx.h>
#endif

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

static gint defaultAttrlist[] = {
  GDK_GL_RGBA,
  GDK_GL_DOUBLEBUFFER,
  GDK_GL_DEPTH_SIZE, 16,
  GDK_GL_NONE
};

namespace Ravl2
{


#if !RAVL_USE_GTKGLEXT
  static GLboolean CheckExtension(const char *extName )
  {
    /*
    ** Search for extName in the extensions string.  Use of strstr()
    ** is not sufficient because extension names can be prefixes of
    ** other extension names.  Could use strtok() but the constant
    ** string returned by glGetString can be in read-only memory.
    */
    char *p = (char *) glGetString(GL_EXTENSIONS);
    char *end;
    int extNameLen;
    
    extNameLen = strlen(extName);
    end = p + strlen(p);
    
    while (p < end) {
      int n = strcspn(p, " ");
      if ((extNameLen == n) && (strncmp(extName, p, n) == 0)) {
        return GL_TRUE;
      }
      p += (n + 1);
    }
    return GL_FALSE;
  }
#endif
  
  //: Create a 3D canvas
  Canvas3DBodyC::Canvas3DBodyC(int x, int y, int *nglattrlist,
                               bool autoConfigure)
    : glattrlist(nglattrlist),
      sx(x),
      sy(y),
      m_eRenderMode(C3D_SMOOTH),
      m_bTexture(false),
      m_bLighting(true),
      m_autoConfigure(autoConfigure),
      m_glExtNonPowerOfTwoTexture(false),
      m_initDone(false)
  {
    ONDEBUG(std::cerr << "Canvas3DBodyC::Canvas3DBodyC(), Called.\n");
#if RAVL_USE_GTKGLEXT
    if(!gtk_gl_init_check(NULL, NULL))
      RavlIssueError("Touble initialising gtk_gl\n");
    if(!gdk_gl_init_check(NULL, NULL))
      RavlIssueError("Trouble initialising gdk_gl\n");  
#endif
  }

  //: Initalise GL info
  bool Canvas3DBodyC::GUIInitGL() {
    ONDEBUG(std::cerr << "Canvas3DBodyC::GUIInitGL(), GL Avaliable ? \n");
	bool ret = false;
#if RAVL_USE_GTKGLEXT
    gint major, minor;
    gdk_gl_query_version(&major, &minor);
    ONDEBUG(std::cerr << "OpenGL extension version - %d.%d\n" << major << " " << minor << endl);
    if(major > 0) ret = true;
#else
    ret = gdk_gl_query();    
    if(!ret) {
#if defined(__sgi__)
      std::cerr << "No native OpenGL supported on this display. \n";
      std::cerr << "You could try: 'setenv LD_LIBRARY_PATH /opt/PDmesa/Mesa-3.1/lib' \n";
      std::cerr << "Then restarting your program. \n";
      RavlAssertMsg(0,"OpenGL not supported. ");
#endif
      ONDEBUG(std::cerr << "OpenGL not found" << endl);
    }
#endif
    return ret;
  }

  //: Create with a widget supplied from elsewhere.
  bool Canvas3DBodyC::Create(GtkWidget *Parent)
  {
    
    // Initalise opengl
    if(!GUIInitGL()) {
#if 0
      throw ExceptionC("OpenGL not supported on display. \n");
#else
      std::cerr << "WARNING: OpenGL not supported on this X server. \n";
#endif
    }
    
    ONDEBUG(std::cerr << "Canvas3DBodyC::Create(GtkWidget *), Setting up canvas. \n");

#if RAVL_USE_GTKGLEXT
    
    widget = gtk_drawing_area_new();
    if(widget == 0) {
      std::cerr << "Canvas3DBodyC::Create(GtkWidget *) ERROR: Widget create failed. \n";
      return false;
    }
    GdkGLConfig *glconfig;
    // Try double-buffered visual
    glconfig = gdk_gl_config_new_by_mode ((GdkGLConfigMode)(GDK_GL_MODE_RGB | GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE));
    if (glconfig == NULL) {
      g_print ("*** Cannot find the double-buffered visual.\n");
      g_print ("*** Trying single-buffered visual.\n");
      
      /* Try single-buffered visual */
      glconfig = gdk_gl_config_new_by_mode ((GdkGLConfigMode)(GDK_GL_MODE_RGB | GDK_GL_MODE_DEPTH));
      if (glconfig == NULL) {
        g_print ("*** No appropriate OpenGL-capable visual found.\n");
        exit (1);
      }
    }
    gtk_widget_set_gl_capability (widget, glconfig, NULL, TRUE, GDK_GL_RGBA_TYPE);	    
#else
    if(glattrlist == 0)
      glattrlist = defaultAttrlist; // Use default.
    
    widget = gtk_gl_area_new(glattrlist);
    if(widget == 0) {
      std::cerr << "Canvas3DBodyC::Create(GtkWidget *) ERROR: Widget create failed. \n";
      return false;
    }
#endif
    
    if(Parent != NULL) {
      // The widget supplied is unlikely to be the correct type,
      // so we assume its a container into which we'll create
      // the graphics context.

      // Add in child widget...
      gtk_container_add (GTK_CONTAINER (Parent), widget);
      //gtk_widget_show (widget);
    }
    else {
      // Setup drawing area.
      ONDEBUG(std::cerr << "Canvas3DBodyC::Create(GtkWidget *), Setting draw area size to " << sx << " " << sy <<". \n");
      gtk_drawing_area_size (GTK_DRAWING_AREA (widget), sx, sy);
    }
    
    // When window is resized viewport needs to be resized also.
    ConnectRef(Signal("configure_event"), *this,&Canvas3DBodyC::CBConfigureEvent);
    
    gtk_quit_add_destroy(1, GTK_OBJECT(widget));

    ONDEBUG(std::cerr << "Canvas3DBodyC::Create(GtkWidget *), Connect Signals. \n");
    ConnectSignals();
    ONDEBUG(std::cerr << "Canvas3DBodyC::Create(GtkWidget *), Complete. \n");

    return true;
  }

  //: Call before using any GL commands.
  bool Canvas3DBodyC::GUIBeginGL()
  {
    if(widget == 0) {
      ONDEBUG(std::cerr << "Canvas3DBodyC::BeginGL(), ERROR: Called with invalid widget. \n");
      return false;
    }
#if RAVL_USE_GTKGLEXT
    GdkGLContext *glcontext = gtk_widget_get_gl_context (widget);
    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);
    if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext))
      return FALSE;
#else
    if (!gtk_gl_area_make_current(GTK_GL_AREA(widget))) {
      ONDEBUG(std::cerr << "WARNING: Canvas3DBodyC::BeginGL(), Failed. \n");
      return false;
    }
    if(!m_initDone) {
      m_initDone = true;
      m_glExtNonPowerOfTwoTexture = CheckExtension("GL_ARB_texture_non_power_of_two");
      ONDEBUG(std::cerr << "Non power of two texture: " << m_glExtNonPowerOfTwoTexture << "\n");
    }
#endif
    return true;
  }

  //: Call after finished with GL
  bool Canvas3DBodyC::GUIEndGL()
  {
#if RAVL_USE_GTKGLEXT
    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);
    gdk_gl_drawable_gl_end (gldrawable);
#endif
    //glFinish();
    //cerr << "WARNING: Canvas3DBodyC::BeginGL(), Failed. \n";
    return true;
  }

  //: swap buffers.
  bool Canvas3DBodyC::GUISwapBuffers()
  {
#if RAVL_USE_GTKGLEXT
    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);    
    if (gdk_gl_drawable_is_double_buffered (gldrawable))
      gdk_gl_drawable_swap_buffers (gldrawable);
    else
      glFlush ();
#else
    gtk_gl_area_swapbuffers(GTK_GL_AREA(widget));
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glLoadIdentity();
#endif
    return true;
  }

  //: clear buffers (make sure you called GUIBeginGL before)
  bool Canvas3DBodyC::GUIClearBuffers()
  {
    GLenum whichBuffers(GL_COLOR_BUFFER_BIT);
    if(glIsEnabled(GL_DEPTH_TEST) )
      whichBuffers |= (GL_DEPTH_BUFFER_BIT);
    glClear(whichBuffers);
    //glMatrixMode(GL_MODELVIEW);
    //glLoadIdentity();
    return true;
  }

  //: Process OpenGL requests.
  bool Canvas3DBodyC::GUIProcessReq(DObject3DC &obj) {
    ONDEBUG(std::cerr << "Canvas3DBodyC::GUIProcessReq(), Called. \n");
    if(!GUIBeginGL()) {
      std::cerr << "Canvas3DBodyC::GUIProcessReq(), Failed to BeginGL(). \n";
      return false;
    }
    Canvas3DC c(*this);
    if(!obj.IsValid())
     std::cerr << "Canvas3DBodyC::GUIProcessReq(), Given invalid object. \n";
    else
      obj.GUIRenderDL(c);
    GUIEndGL();
    return true;
  }

  //: Put render instruction into pipe.
  bool Canvas3DBodyC::Put(const DObject3DC &obj) {
    Manager.Queue(Trigger(Canvas3DC(*this), &Canvas3DC::GUIProcessReq, obj));
    return true;
  }

  //: Handle configure event
  bool Canvas3DBodyC::CBConfigureEvent(GdkEvent *event) {
    ONDEBUG(std::cerr << "Canvas3DBodyC::CBConfigureEvent, Called. ");
    if(!GUIBeginGL())
      return false;
    if(m_autoConfigure) {
      ONDEBUG(std::cerr << "Reshape. " << widget->allocation.width << " " << widget->allocation.height << "\n");
      glViewport(0, 0, widget->allocation.width, widget->allocation.height);
    }
    
    GUIEndGL();
    return true;
  }

  //: Enable or disable lighting
  //: Put End Of Stream marker.
  bool Canvas3DBodyC::SetLightingMode(bool& bLighting) {
    m_bLighting = bLighting;
    Put(DOpenGLC(CallMethod0C<Canvas3DC, bool>(Canvas3DC(*this), &Canvas3DC::GUIDoLighting)));
    return true;
  }
  
  //: Write contents of widget to an image.
  
  bool Canvas3DBodyC::SaveToImageInternal(Ravl2::Array<ByteRGBValueC,2> *img,SemaphoreRC &done) {
    if(!GUIBeginGL()) {
      std::cerr << "Failed to begin context.";
      return false;
    }
    GLint x=0,y=0;
    GLsizei width=Size()[1],height=Size()[0];
    std::cerr << "Canvas3DBodyC::SaveToImage, Width=" << width << " Height=" << height << "\n";
    width -= width%4;
    GLenum format=GL_RGB, wtype=GL_UNSIGNED_BYTE;
    Ravl2::Array<ByteRGBValueC,2> out(height,width);
    GLvoid* buf = (GLvoid*) &(out(0,0));
    glReadPixels(x,y,width,height,format,wtype,buf);
    Ravl2::ReflectHorizontal(out,*img);
    GUIEndGL();
    done.Post();
    return true;    
  }

  //: Write contents of screen to an image.
  
  bool Canvas3DBodyC::SaveToImage(Ravl2::Array<ByteRGBValueC,2> &img) {
    SemaphoreRC done(0);
    if(!Manager.IsGUIThread()) {
      Manager.Queue(TriggerR(*this,&Canvas3DBodyC::SaveToImageInternal,&img,done));
      done.Wait();
      return true;
    }
    return SaveToImageInternal(&img,done);
  }

} // end of namespace


