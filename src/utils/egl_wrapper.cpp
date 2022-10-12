#include <cstdlib>
#include <iostream>

#include "egl_wrapper.h"
#include "log.h"

#define EGL_CHECK_RESULT(X) do {\
    EGLint error = eglGetError(); \
    if(!(X) || error != EGL_SUCCESS) { \
        LOGE("EGL error %d at %s : %d\n", error, __FILE__, __LINE__); \
        std::exit(1); \
    } \
} while(0)

EglWrapper::EglWrapper()
{
    create_context();
}

EglWrapper::~EglWrapper()
{
    release_context();
}

void EglWrapper::create_context()
{
    LOGI("Creat context.");

    const EGLint window_attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        // EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_NONE
    };

    const EGLint surface_attribs[] = {
        EGL_WIDTH, 512,
        EGL_HEIGHT, 512,
        EGL_LARGEST_PBUFFER, EGL_TRUE,
        EGL_NONE
    };

    const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    // get an EGL display connection
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGL_CHECK_RESULT(display_ != EGL_NO_DISPLAY);

    // initialize the EGL display connection
    EGLint major_version, minor_version;
    EGL_CHECK_RESULT(eglInitialize(display_, &major_version, &minor_version));
    LOGI("EGL version: %d.%d", major_version, minor_version);

    // get an appropriate EGL frame buffer configuration
    EGLConfig config;
    EGLint num_config;
    EGL_CHECK_RESULT(eglChooseConfig(display_, window_attribs, &config, 1, &num_config));

    // create a pixel buffer (pbuffer) surface
    surface_ = eglCreatePbufferSurface(display_, config, surface_attribs);
    EGL_CHECK_RESULT(surface_ != EGL_NO_SURFACE);

    EGLint width, height;
    eglQuerySurface(display_, surface_, EGL_WIDTH, &width);
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &height);
    LOGI("Surface dimension: (%d, %d)", height, width);

    // create an EGL rendering context
    context_ = eglCreateContext(display_, config, EGL_NO_CONTEXT, context_attribs);
    EGL_CHECK_RESULT(context_ != EGL_NO_CONTEXT);

    // connect the context to the surface
    EGL_CHECK_RESULT(eglMakeCurrent(display_, surface_, surface_, context_));
}

void EglWrapper::release_context()
{
    LOGI("Release context.");

    EGL_CHECK_RESULT(eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
    EGL_CHECK_RESULT(eglDestroyContext(display_, context_));
    EGL_CHECK_RESULT(eglDestroySurface(display_, surface_));
    EGL_CHECK_RESULT(eglTerminate(display_));

    display_ = EGL_NO_DISPLAY;
    context_ = EGL_NO_CONTEXT;
    surface_ = EGL_NO_SURFACE;
}