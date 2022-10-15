#ifndef AB6D0549_6DD8_4331_8203_00636094CC8B
#define AB6D0549_6DD8_4331_8203_00636094CC8B

#include <EGL/egl.h>

class EglWrapper
{
public:
    EglWrapper();
    ~EglWrapper();

    EGLDisplay get_display() { return display_; }
    EGLContext get_context() { return context_; }
    EGLSurface get_surface() { return surface_; }
private:
    void create_context();
    void release_context();

    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLContext context_ = EGL_NO_CONTEXT;
    EGLSurface surface_ = EGL_NO_SURFACE;
};

#endif /* AB6D0549_6DD8_4331_8203_00636094CC8B */
