#include <android_native_app_glue.h>


#include "Manager/Renderer/RendererManager.h"
#include "Manager/Thread/ThreadManager.h"

#include <thread>

void android_main(struct android_app* state) {
    ThreadManager threadManager;
    RendererManager rendererManager(state);
}
