//
// Created by sweetgrape on 2022/08/07.
//

#include "RenderingEngine.h"

void RenderingEngine::engine(struct android_app* state) {
    //https://developer.android.com/reference/games/game-activity/struct/android-app

    struct engine engine{};

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    if (state->savedState != nullptr) {
        // We are starting with a previous saved state; restore from it.
        engine.state = *(struct saved_state*)state->savedState;
    }

    // loop waiting for stuff to do.

    while (true) {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident=ALooper_pollAll(engine.animating ? 0 : -1, nullptr, &events,
                                      (void**)&source)) >= 0) {

            // Process this event.
            if (source != nullptr) {
                source->process(state, source);
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }

        if (engine.animating) {
            // Done with events; draw next animation frame.
            engine.state.angle += .01f;
            if (engine.state.angle > 1) {
                engine.state.angle = 0;
            }

            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            engine_draw_frame(&engine);
        }
    }
}

int RenderingEngine::engine_init_display(struct engine *engine) {
    // initialize OpenGL ES and EGL

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
    };
    EGLint w, h, format;
    EGLint numConfigs;
    EGLConfig config = nullptr;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, nullptr, nullptr);

    /* Here, the application chooses the configuration it desires.
     * find the best match if possible, otherwise use the very first one
     */
    eglChooseConfig(display, attribs, nullptr,0, &numConfigs);
    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    assert(supportedConfigs);
    eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);
    assert(numConfigs);
    auto i = 0;
    for (; i < numConfigs; i++) {
        auto& cfg = supportedConfigs[i];
        EGLint r, g, b, d;
        if (eglGetConfigAttrib(display, cfg, EGL_RED_SIZE, &r)   &&
            eglGetConfigAttrib(display, cfg, EGL_GREEN_SIZE, &g) &&
            eglGetConfigAttrib(display, cfg, EGL_BLUE_SIZE, &b)  &&
            eglGetConfigAttrib(display, cfg, EGL_DEPTH_SIZE, &d) &&
            r == 8 && g == 8 && b == 8 && d == 0 ) {

            config = supportedConfigs[i];
            break;
        }
    }
    if (i == numConfigs) {
        config = supportedConfigs[0];
    }

    if (config == nullptr) {
        LOGW("Unable to initialize EGLConfig");
        return -1;
    }

    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    surface = eglCreateWindowSurface(display, config, engine->app->window, nullptr);
    context = eglCreateContext(display, config, nullptr, nullptr);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGW("Unable to eglMakeCurrent");
        return -1;
    }

    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;
    engine->state.angle = 0;

    // Check openGL on the system
    auto opengl_info = {GL_VENDOR, GL_RENDERER, GL_VERSION, GL_EXTENSIONS};
    for (auto name : opengl_info) {
        auto info = glGetString(name);
        LOGI("OpenGL Info: %s", info);
    }
    // Initialize GL state.
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);

    return 0;
}

void RenderingEngine::engine_draw_frame(struct engine *engine) {
    if (engine->display == nullptr) {
        // No display.
        return;
    }

    // Just fill the screen with a color.
    glClearColor(((float)engine->state.x)/engine->width,
                 engine->state.angle,
                 ((float)engine->state.y)/engine->height,
                 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* 현재의 매트릭스 모드를 GL_PROJECTION로 설정 */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* 직교 투영으로 좌표계의 범위를 설정한다. */
    GLfloat ratio = 3;
    GLfloat right = (GLfloat)engine->width / engine->height * ratio;
    GLfloat top = ratio;
    glOrthof(-right, right, -top, top, -10.0f, 10.0f);

    /* 현재의 매트릭스 모드를 GL_MODELVIEW로 설정 */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* 출력될 도형을 위한 좌표 설정 */
    GLfloat points[] = {
            -0.5f,-0.5f,-0.5f,    /* 뒤좌하*/
            -0.5f, 0.5f,-0.5f,    /* 뒤좌상 */
            0.5f, 0.5f,-0.5f,    /* 뒤우상 */
            0.5f,-0.5f,-0.5f,    /* 뒤우하 */
            -0.5f,-0.5f, 0.5f,    /* 앞좌하 */
            -0.5f, 0.5f, 0.5f,    /* 앞좌상 */
            0.5f, 0.5f, 0.5f,    /* 앞우상 */
            0.5f,-0.5f, 0.5f };   /* 앞우하 */

    /* 12개의 삼각형을 위한 정점의 배열 인덱스 */
    GLubyte indices[] = {
            0,1,2, 0,2,3,    /* 뒤면 */
            4,6,5, 4,7,6,    /* 앞면 */
            0,4,5, 0,5,1,    /* 좌면 */
            1,5,6, 1,6,2,    /* 윗면 */
            2,6,7, 2,7,3,    /* 우면 */
            3,7,4, 3,4,0 };   /* 하면 */
    /* 정점 배열 사용을 설정 */
    glEnableClientState(GL_VERTEX_ARRAY);

    /* 그래픽 출력을 위한 정점 배열 사용에 대한 설정 */
    glRotatef(engine->state.x, 1, 0, 0);
    glRotatef(engine->state.y, 0, 1, 0);
    glVertexPointer(3, GL_FLOAT, 0, points);

    glColor4f(((float)engine->state.y)/engine->height,
              ((float)engine->state.x)/engine->width,
              engine->state.angle,
              1);
    /* 삼각형 12개를 드로잉(6*2) : 처리할 정점의 수 : 36개 */
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, indices);

    /* 그래픽 출력을 하고 나서 정점 배열 사용에 대한 설정 해제 */
    glDisableClientState(GL_VERTEX_ARRAY);

    eglSwapBuffers(engine->display, engine->surface);
}

void RenderingEngine::engine_term_display(struct engine *engine) {
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }
    engine->animating = 0;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}

int32_t RenderingEngine::engine_handle_input(struct android_app *app, AInputEvent *event) {
    auto* engine = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        engine->animating = 1;
        engine->state.x = AMotionEvent_getX(event, 0);
        engine->state.y = AMotionEvent_getY(event, 0);
        return 1;
    }
    return 0;
}

void RenderingEngine::engine_handle_cmd(struct android_app *app, int32_t cmd) {
    auto* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (engine->app->window != nullptr) {
                engine_init_display(engine);
                engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            break;
        case APP_CMD_LOST_FOCUS:
            break;
        default:
            break;
    }
}
