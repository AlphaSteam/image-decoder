// JNI wrapper that attempts to dlopen libvips at runtime and call vips_init/vips_shutdown.
// This allows building and running even when libvips isn't bundled; nativeInit will
// return false if libvips can't be loaded.

#include <jni.h>
#include <dlfcn.h>
#include <android/log.h>
#include <string>

static void* vips_handle = nullptr;
typedef int (*vips_init_t)(const char* argv0);
static vips_init_t vips_init_ptr = nullptr;
typedef void (*vips_shutdown_t)();
static vips_shutdown_t vips_shutdown_ptr = nullptr;

#define LOG_TAG "vipswrapper-jni"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" JNIEXPORT jboolean JNICALL
Java_tachiyomi_decoder_VipsWrapper_nativeInit(JNIEnv* env, jclass clazz) {
    if (vips_handle != nullptr) {
        // Already initialised
        return JNI_TRUE;
    }

    const char* candidates[] = {"libvips.so", "libvips.so.42", "libvips"};
    for (const char* name : candidates) {
        vips_handle = dlopen(name, RTLD_NOW | RTLD_GLOBAL);
        if (vips_handle) break;
    }

    if (!vips_handle) {
        const char* err = dlerror();
        LOGE("dlopen libvips failed: %s", err ? err : "(unknown)");
        return JNI_FALSE;
    }

    vips_init_ptr = (vips_init_t)dlsym(vips_handle, "vips_init");
    vips_shutdown_ptr = (vips_shutdown_t)dlsym(vips_handle, "vips_shutdown");
    if (!vips_init_ptr || !vips_shutdown_ptr) {
        LOGE("Failed to find vips_init or vips_shutdown symbols");
        dlclose(vips_handle);
        vips_handle = nullptr;
        vips_init_ptr = nullptr;
        vips_shutdown_ptr = nullptr;
        return JNI_FALSE;
    }

    int rc = vips_init_ptr("vipswrapper");
    if (rc != 0) {
        LOGE("vips_init returned error: %d", rc);
        dlclose(vips_handle);
        vips_handle = nullptr;
        vips_init_ptr = nullptr;
        vips_shutdown_ptr = nullptr;
        return JNI_FALSE;
    }

    LOGI("libvips loaded and initialised via dlopen");
    return JNI_TRUE;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_tachiyomi_decoder_VipsWrapper_nativeResizeFromMemory(JNIEnv* env, jclass clazz,
                                                          jbyteArray inBuf, jint inW, jint inH,
                                                          jint channels, jint outW, jint outH,
                                                          jint algorithmCode) {
    // Not implemented yet in native wrapper. Return null to indicate caller should fall back.
    (void)inBuf; (void)inW; (void)inH; (void)channels; (void)outW; (void)outH; (void)algorithmCode;
    if (!vips_handle) return nullptr;
    // TODO: implement actual vips resize path here.
    return nullptr;
}

extern "C" JNIEXPORT void JNICALL
Java_tachiyomi_decoder_VipsWrapper_nativeShutdown(JNIEnv* env, jclass clazz) {
    if (vips_shutdown_ptr) {
        vips_shutdown_ptr();
    }
    if (vips_handle) {
        dlclose(vips_handle);
    }
    vips_handle = nullptr;
    vips_init_ptr = nullptr;
    vips_shutdown_ptr = nullptr;
    LOGI("libvips shutdown and handle closed");
}
