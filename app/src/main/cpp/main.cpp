#include <android_native_app_glue.h>


#include "Manager/Renderer/RenderingEngine.h"
#include "Manager/Thread/ThreadManager.h"

#include <thread>

void android_main(struct android_app* state) {
    ThreadManager threadManager;
    RenderingEngine renderingEngine;
    renderingEngine.engine(state);
}
