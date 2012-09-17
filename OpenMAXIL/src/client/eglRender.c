/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <eglRender.h>

/***********************************************************************************
  Function Name      : handle_egl_error
Inputs             : None
Outputs            : None
Returns            : None
Description        : Displays an error message for an egl error
 ************************************************************************************/
static void handle_egl_error(char *name)
{
    EGLint error_code = eglGetError();

    fprintf (stderr, "'%s' returned egl error '%s' (0x%x)\n",
            name,
            egl_error_strings[error_code-EGL_SUCCESS],
            error_code);
    exit(-1);
}

int OpenWindow(RenderContext * renderContext)	
{
    WindowContext * wndContext = malloc(sizeof(WindowContext));
    memset(wndContext,0,sizeof(WindowContext));
    renderContext->wndContext = wndContext;
    if(renderContext->disType == Display_FB){
        wndContext->dis= EGL_DEFAULT_DISPLAY;
        wndContext->win = open("/dev/fb0", O_RDWR);
        if(!wndContext->win){
            printf("Open fb0 failed\n");
            return 0;
        }
    }	else if(renderContext->disType == Display_X11){
        Display * dis;
        dis = XOpenDisplay(NULL);
        if(!dis) {
            printf("display open failed!\n");
            return 0;
        }
        wndContext->win = XCreateSimpleWindow(	dis,RootWindow(dis, 0), 
                1, 1, renderContext->width, renderContext->height, 0,
                BlackPixel (dis, 0), BlackPixel(dis, 0));
        XMapWindow(dis, wndContext->win);
        wndContext->wmDeleteMessage = XInternAtom(dis, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(dis, wndContext->win, &wndContext->wmDeleteMessage, 1);
        XFlush(dis);
        wndContext->dis= dis;
    }
    renderContext->hWnd = wndContext->win ;
    return 1;
}

int CloseWindow(RenderContext * renderContext)
{
    WindowContext * wndContext = renderContext->wndContext;
    printf("Destory Window\n");
    if(renderContext->disType == Display_FB){
        close(wndContext->win); 
        wndContext->win = NULL;
    }else if(renderContext->disType == Display_X11){
        XDestroyWindow(wndContext->dis, wndContext->win);
        XFlush(wndContext->dis);
        XCloseDisplay(wndContext->dis);
    }
    if(renderContext->wndContext){
        free(renderContext->wndContext);
        renderContext->wndContext = NULL;
    }
    return 0;
}

//
//    Log an error message to the debug output for the platform
//
void LogMessage ( const char *formatStr, ... )
{
    va_list params;
    char buf[BUFSIZ];

    va_start ( params, formatStr );
    vsprintf( buf,formatStr, params );

    printf ( "%s", buf );

    va_end ( params );
}

EGLBoolean CreateEGLContext ( RenderContext* pRenderContext, EGLDisplay* pDisplay,
        EGLContext* pContext, EGLSurface* pWindowSurface,
        EGLint attribList[])
{
    EGLint numConfigs;
    EGLint majorVersion;
    EGLint minorVersion;
    EGLConfig eglConfig;
    EGLDisplay eglDisplay;
    EGLContext eglContext;
    EGLSurface eglWindowSurface;
    EGLBoolean ret;

    static const EGLint	gl_context_attribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION,	2,
        EGL_NONE
    };

    eglBindAPI(EGL_OPENGL_ES_API);

    WindowContext * wndContext = pRenderContext->wndContext;
    eglDisplay = eglGetDisplay(wndContext->dis);
    eglInitialize(eglDisplay, &majorVersion, &minorVersion);

    eglGetConfigs(eglDisplay, NULL, 1, &numConfigs);

    eglChooseConfig(eglDisplay, attribList, &eglConfig, 1, &numConfigs);
    printf("%s:eglChooseConfig\n", __func__);

    eglWindowSurface = eglCreateWindowSurface(eglDisplay, eglConfig, wndContext->win, NULL);

    if (eglWindowSurface == EGL_NO_SURFACE)
    {
        handle_egl_error("eglCreateWindowSurface");
    } else{
        printf("%s:eglCreateWindowSurface ok\n", __func__);
    }

    eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, gl_context_attribs);
    if (eglContext == EGL_NO_CONTEXT)
    {
        handle_egl_error("eglCreateContext");
    }else{
        printf("%s:eglCreateContext ok\n", __func__);
    }

    ret = eglMakeCurrent(eglDisplay, eglWindowSurface, eglWindowSurface, eglContext);

    if (ret == EGL_FALSE)
        handle_egl_error("eglMakeCurrent");
    else
        printf("%s:eglMakeCurrent ok\n", __func__);

    if(pRenderContext->preserved)
    {
        eglSurfaceAttrib(eglDisplay, eglWindowSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED);
    }
    *pDisplay = eglDisplay;
    *pWindowSurface = eglWindowSurface;
    *pContext = eglContext;

    return EGL_TRUE;
}


static const EGLint s_X11configAttribs[] =
{
    EGL_RED_SIZE,		5,
    EGL_GREEN_SIZE, 	6,
    EGL_BLUE_SIZE,		5,
    EGL_ALPHA_SIZE, 	EGL_DONT_CARE,
    EGL_SURFACE_TYPE,	EGL_WINDOW_BIT | EGL_SWAP_BEHAVIOR_PRESERVED_BIT,
    EGL_NONE
};

static const EGLint s_FBconfigAttribs[] =
{
    EGL_RED_SIZE,       5,
    EGL_GREEN_SIZE,     6,
    EGL_BLUE_SIZE,      5,
    EGL_ALPHA_SIZE,     EGL_DONT_CARE,
    EGL_DEPTH_SIZE,     EGL_DONT_CARE,
    EGL_STENCIL_SIZE,   EGL_DONT_CARE,
    EGL_SAMPLE_BUFFERS, 0,
    EGL_NONE
};



int eglOpen(RenderContext * renderContext)
{
    EGLBoolean ret;
    int preserved;
    EGLint s_configAttribs;

    static EGLint s_pbufferAttribs[] =
    {
        EGL_WIDTH,  0,
        EGL_HEIGHT, 0,
        EGL_NONE
    };

    s_pbufferAttribs[1]=480;
    s_pbufferAttribs[3]=640;

    if (!OpenWindow(renderContext))
        return -1;	 

    if(renderContext->disType == Display_FB)
        s_configAttribs = s_FBconfigAttribs;
    else if(renderContext->disType == Display_X11)
        s_configAttribs = s_X11configAttribs;

    CreateEGLContext(	renderContext,
            &renderContext->eglDisplay,
            &renderContext->eglContext,
            &renderContext->eglWindowSurface,
            s_configAttribs);  	
    Init(renderContext);
    return 0 ;  	

}

void eglClose(RenderContext * renderContext)
{
    /* Destroy all EGL resources */
    eglMakeCurrent(renderContext->eglDisplay, NULL, NULL, NULL);
    eglDestroyContext(renderContext->eglDisplay, renderContext->eglContext);
    eglDestroySurface(renderContext->eglDisplay, renderContext->eglWindowSurface);

    if(renderContext->eglImage[0])
        eglDestroyImageKHR(renderContext->eglDisplay,renderContext->eglImage[0]);
    if(renderContext->eglImage[1])
        eglDestroyImageKHR(renderContext->eglDisplay,renderContext->eglImage[1]);

    WindowContext * wndContext = renderContext->wndContext;	
    if(renderContext->pixmap[0])
        XFreePixmap(wndContext->dis,renderContext->pixmap[0]);	
    if(renderContext->pixmap[1])
        XFreePixmap(wndContext->dis,renderContext->pixmap[1]);

    eglTerminate(renderContext->eglDisplay);
    CloseWindow(renderContext);
    if(renderContext->wndContext){
        free(renderContext->wndContext);
        renderContext->wndContext = NULL;
    }
}

int RoundUpPower2( int in_num)
{
    int out_num = in_num;
    int n;
    for (n = 0; out_num != 0; n++)
        out_num >>= 1;
    out_num = 1 << n;
    if (out_num == 2 * in_num)
        out_num >>= 1;
    return out_num;	
}

GLuint LoadShader ( GLenum type, const char *shaderSrc )
{
    GLuint shader;
    GLint compiled;

    // Create the shader object
    shader = glCreateShader ( type );

    if ( shader == 0 )
        return 0;

    // Load the shader source
    glShaderSource ( shader, 1, &shaderSrc, NULL );

    // Compile the shader
    glCompileShader ( shader );

    // Check the compile status
    glGetShaderiv ( shader, GL_COMPILE_STATUS, &compiled );

    if ( !compiled ) 
    {
        GLint infoLen = 0;

        glGetShaderiv ( shader, GL_INFO_LOG_LENGTH, &infoLen );

        if ( infoLen > 1 )
        {
            char* infoLog = malloc (sizeof(char) * infoLen );

            glGetShaderInfoLog ( shader, infoLen, NULL, infoLog );
            LogMessage ( "Error compiling shader:\n%s\n", infoLog );            

            free ( infoLog );
        }

        glDeleteShader ( shader );
        return 0;
    }

    return shader;

}

void * CreateEGLImage(RenderContext * renderContext)
{
    // Texture object handle
    GLuint textureId;
    EGLImageKHR eglImage;

    renderContext->Texwidth = RoundUpPower2(renderContext->width);
    renderContext->Texheight = RoundUpPower2(renderContext->height);

    // Use tightly packed data
    glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );

    // Generate a texture object
    glGenTextures ( 1, &textureId );

    // Bind the texture object
    glBindTexture ( GL_TEXTURE_2D, textureId );	

    // Set the filtering mode
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    GLint nIdx = renderContext->texIndex;

    int Bpp = 16;
    WindowContext * wndContext = renderContext->wndContext;
    renderContext->pixmap[nIdx] = XCreatePixmap(
            wndContext->dis,
            wndContext->win,
            renderContext->Texwidth,
            renderContext->Texheight,
            Bpp);

    printf("width %d, height %d ,pixmap %p\n",renderContext->Texwidth,\
            renderContext->Texheight,renderContext->pixmap[nIdx]);

    eglImage = eglCreateImageKHR(
            renderContext->eglDisplay,
            renderContext->eglContext,
            EGL_NATIVE_PIXMAP_KHR,
            (void*)renderContext->pixmap[nIdx],
            NULL);

    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eglImage); 
    renderContext->eglImage[nIdx] = eglImage;	

    printf("nIdx: %d, textureId: %lld, eglImage: %x\n", nIdx, textureId, eglImage);

    renderContext->textureId[nIdx] = textureId;
    renderContext->CurrentTexIndex =  nIdx;
    renderContext->texIndex++;	
    return renderContext->eglImage[nIdx] ;
}

///
// Initialize the shader and program object
//
int Init ( RenderContext *renderContext )
{
    GLuint vsShader = 0, fsShader = 0;
    GLuint program = 0;
    GLint linked; 	

    GLbyte vShaderStr[] =  
        "attribute vec4 a_position;   \n"
        "attribute vec2 a_texCoord;   \n"
        "varying vec2 v_texCoord;     \n"
        "void main()                  \n"
        "{                            \n"
        "   gl_Position = a_position; \n"
        "   v_texCoord = a_texCoord;  \n"
        "}                            \n";

    GLbyte fShaderStr[] =  
        "precision mediump float;                            \n"
        "varying vec2 v_texCoord;                            \n"
        "uniform sampler2D s_texture;                        \n"
        "void main()                                         \n"
        "{                                                   \n"
        "  gl_FragColor = texture2D( s_texture, v_texCoord );\n"
        "}                                                   \n";


    // Load the shaders and get a linked program object
    vsShader = LoadShader(GL_VERTEX_SHADER,vShaderStr);
    fsShader = LoadShader(GL_FRAGMENT_SHADER,fShaderStr);
    program = glCreateProgram();
    if (program == 0)
    {
        return 0;
    }

    glAttachShader(program, vsShader);
    glAttachShader(program, fsShader);
    glLinkProgram(program);

    // Check the link status
    glGetProgramiv ( program, GL_LINK_STATUS, &linked );

    if ( !linked ) 
    {
        GLint infoLen = 0;

        glGetProgramiv ( program, GL_INFO_LOG_LENGTH, &infoLen );

        if ( infoLen > 1 )
        {
            char* infoLog = malloc (sizeof(char) * infoLen );

            glGetProgramInfoLog ( program, infoLen, NULL, infoLog );
            LogMessage ( "Error linking program:\n%s\n", infoLog );            

            free ( infoLog );
        }

        glDeleteProgram ( program );
        return 0;
    }

    // Free up no longer needed shader resources
    glDeleteShader ( vsShader );
    glDeleteShader ( fsShader );		

    renderContext->programObject = program ;
    // Get the attribute locations
    renderContext->positionLoc = glGetAttribLocation ( program, "a_position" );
    renderContext->texCoordLoc = glGetAttribLocation ( program, "a_texCoord" );

    // Get the sampler location
    renderContext->samplerLoc = glGetUniformLocation ( program, "s_texture" );

    // Load the texture
    //renderContext->textureId[textureIndex] = CreateTexture2D (renderContext);

    glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
    return 1;
}


int find_texture_from_eglimage(void * renderContext, EGLImageKHR eglImage)
{

    RenderContext * InternalContext = renderContext;

    if(eglImage){
        GLint i;
        for(i=0;i<2;i++){
            if(InternalContext->eglImage[i]== eglImage )	{
                InternalContext->CurrentTexIndex = i;
                break;
            }
        }
        if(i==2){
            GLint texIndex = InternalContext->texIndex;
            if(texIndex ==2){
                printf("Error\n");
                return -1;
            }
        }
    }
    return 0;
}
///
// Draw a triangle using the shader pair created in Init()
//
void Draw ( RenderContext *renderContext, EGLImageKHR eglImage )
{

    GLfloat vVertices[] = { -1.0f,  1.0f, 0.0f,  // Position 0
        0.0f,  0.0f,        // TexCoord 0 
        -1.0f, -1.0f, 0.0f,  // Position 1
        0.0f,  1.0f,        // TexCoord 1
        1.0f, -1.0f, 0.0f,  // Position 2
        1.0f,  1.0f,        // TexCoord 2
        1.0f,  1.0f, 0.0f,  // Position 3
        1.0f,  0.0f         // TexCoord 3
    };

    GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
    vVertices[9] = vVertices[14] = (GLfloat)renderContext->height /renderContext->Texheight;
    vVertices[13] = vVertices[18] = (GLfloat)renderContext->width /renderContext->Texwidth;
    // Set the viewport
    glViewport ( 0, 0, renderContext->width, renderContext->height );

    // Clear the color buffer
    glClear ( GL_COLOR_BUFFER_BIT );

    // Use the program object
    glUseProgram ( renderContext->programObject );

    // Load the vertex position
    glVertexAttribPointer ( renderContext->positionLoc, 3, GL_FLOAT, 
            GL_FALSE, 5 * sizeof(GLfloat), vVertices );
    // Load the texture coordinate
    glVertexAttribPointer ( renderContext->texCoordLoc, 2, GL_FLOAT,
            GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3] );

    glEnableVertexAttribArray ( renderContext->positionLoc );
    glEnableVertexAttribArray ( renderContext->texCoordLoc );

    // Bind the texture
    glActiveTexture ( GL_TEXTURE0 );
    find_texture_from_eglimage(renderContext,eglImage);
    glBindTexture ( GL_TEXTURE_2D, renderContext->textureId[renderContext->CurrentTexIndex] );

    // Set the sampler texture unit to 0
    glUniform1i ( renderContext->samplerLoc, 0 );

    glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );

    eglSwapBuffers ( renderContext->eglDisplay, renderContext->eglWindowSurface );
}

