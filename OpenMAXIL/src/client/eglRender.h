/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */
#include <EGL/eglplatform.h>
#include "EGL/egl.h"
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2amdext.h>

#include <X11/Xlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/mman.h>

typedef struct
{
	GLuint size;
	GLubyte * phyAddr;
	GLubyte * virtAddr;	
}PixelsLoc;
static char * const egl_error_strings[] =
{
	"EGL_SUCCESS",
    "EGL_NOT_INITIALIZED",
    "EGL_BAD_ACCESS",
    "EGL_BAD_ALLOC",
    "EGL_BAD_ATTRIBUTE",    
    "EGL_BAD_CONFIG",
    "EGL_BAD_CONTEXT",   
    "EGL_BAD_CURRENT_SURFACE",
    "EGL_BAD_DISPLAY",
    "EGL_BAD_MATCH",
    "EGL_BAD_NATIVE_PIXMAP",
    "EGL_BAD_NATIVE_WINDOW",
    "EGL_BAD_PARAMETER",
    "EGL_BAD_SURFACE"
};

typedef struct
{
	NativePixmapType *dis;
 	NativeWindowType win;
 	Atom wmDeleteMessage;
} WindowContext;

typedef enum
{
  Display_NONE =-1,
  Display_X11,	
  Display_FB
}DisplayType;

typedef struct
{
	 DisplayType disType;
   void * wndContext;	
   /// Window width
   GLint       width;

   /// Window height
   GLint       height;

   /// Window width
   GLint       Texwidth;

   /// Window height
   GLint       Texheight;

   /// Window handle
   EGLNativeWindowType  hWnd;

   /// EGL display
   EGLDisplay  eglDisplay;
      
   /// EGL context
   EGLContext  eglContext;

   /// EGL surface
   EGLSurface  eglWindowSurface;


   // Handle to a program object
   GLuint programObject;

   // Attribute locations
   GLint  preserved;
   GLint  positionLoc;
   GLint  texCoordLoc;
   GLint  colorLoc;

   // Sampler location
   GLint samplerLoc;

   // Texture handle
   GLint texIndex;
   GLint CurrentTexIndex;
   GLuint textureId[2];
   EGLImageKHR eglImage[2];
   Pixmap pixmap[2];
} RenderContext;

#ifdef __cplusplus
extern "C" {
#endif

int RoundUpPower2( int in_num);
int eglOpen(RenderContext * renderContext);
void eglClose(RenderContext * renderContext);
void SetPixelsLoc(void *renderContext , PixelsLoc * pixelsLoc);
void Draw ( RenderContext *renderContext,  EGLImageKHR eglImage);
void * CreateEGLImage(RenderContext * renderContext);

#ifdef __cplusplus
}
#endif

