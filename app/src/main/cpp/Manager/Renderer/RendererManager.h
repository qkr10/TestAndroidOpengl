//
// Created by sweetgrape on 2022/08/07.
//

#ifndef MYAPPLICATION3_RENDERERMANAGER_H
#define MYAPPLICATION3_RENDERERMANAGER_H

#include <initializer_list>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <jni.h>
#include <cerrno>
#include <cassert>

#include <dlfcn.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

/**
 * Our saved state data.
 */
struct saved_state {
    float angle;
    int32_t x;
    int32_t y;
};

/**
 * Shared state for our app.
 */
struct engine {
    struct android_app* app;

    int animating;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;
    struct saved_state state;
};

class RendererManager {
private:
    static int engine_init_display(struct engine* engine);
    static void engine_draw_frame(struct engine* engine);
    static void engine_term_display(struct engine* engine);
    static int32_t engine_handle_input(struct android_app* app, AInputEvent* event);
    static void engine_handle_cmd(struct android_app* app, int32_t cmd);
public:
    RendererManager(struct android_app* state);
};


#endif //MYAPPLICATION3_RENDERERMANAGER_H
