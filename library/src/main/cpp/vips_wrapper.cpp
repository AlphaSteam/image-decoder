// JNI wrapper that attempts to dlopen libvips at runtime and call vips_init/vips_shutdown.
// This allows building and running even when libvips isn't bundled; nativeInit will
// return false if libvips can't be loaded.

#include <jni.h>
#include <dlfcn.h>
#include <android/log.h>
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>

static void* vips_handle = nullptr;
typedef int (*vips_init_t)(const char* argv0);
static vips_init_t vips_init_ptr = nullptr;
typedef void (*vips_shutdown_t)();
static vips_shutdown_t vips_shutdown_ptr = nullptr;

typedef struct _VipsImage VipsImage;

// Minimal enum definitions copied from libvips headers.
enum VipsBandFormat {
    VIPS_FORMAT_NOTSET = -1,
    VIPS_FORMAT_UCHAR = 0,
};

enum VipsKernel {
    VIPS_KERNEL_NEAREST = 0,
    VIPS_KERNEL_LINEAR = 1,
    VIPS_KERNEL_CUBIC = 2,
    VIPS_KERNEL_MITCHELL = 3,
    VIPS_KERNEL_LANCZOS2 = 4,
    VIPS_KERNEL_LANCZOS3 = 5,
    VIPS_KERNEL_LANCZOS4 = 6,
    VIPS_KERNEL_NOHALO = 7,
    VIPS_KERNEL_VSHARP = 8,
    VIPS_KERNEL_VSQBS = 9,
};

typedef VipsImage* (*vips_image_new_from_memory_t)(const void* data, size_t size,
                                                   int width, int height, int bands,
                                                   int format);
static vips_image_new_from_memory_t vips_image_new_from_memory_ptr = nullptr;

typedef int (*vips_resize_t)(VipsImage* in, VipsImage** out, double scale, ...);
static vips_resize_t vips_resize_ptr = nullptr;

typedef void* (*vips_image_write_to_memory_t)(VipsImage* image, size_t* size_out);
static vips_image_write_to_memory_t vips_image_write_to_memory_ptr = nullptr;

typedef const char* (*vips_error_buffer_t)(void);
static vips_error_buffer_t vips_error_buffer_ptr = nullptr;

typedef void (*vips_error_clear_t)(void);
static vips_error_clear_t vips_error_clear_ptr = nullptr;

typedef int (*vips_image_get_width_t)(const VipsImage* image);
static vips_image_get_width_t vips_image_get_width_ptr = nullptr;

typedef int (*vips_image_get_height_t)(const VipsImage* image);
static vips_image_get_height_t vips_image_get_height_ptr = nullptr;

typedef void (*g_object_unref_t)(void* object);
static g_object_unref_t g_object_unref_ptr = nullptr;

typedef void (*g_free_t)(void* data);
static g_free_t g_free_ptr = nullptr;

#define LOG_TAG "vipswrapper-jni"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static bool load_symbol(void** target, const char* name) {
    *target = dlsym(vips_handle, name);
    if (!*target) {
        LOGE("Failed to resolve symbol %s", name);
        return false;
    }
    return true;
}

static bool ensure_vips_symbols() {
    return load_symbol(reinterpret_cast<void**>(&vips_image_new_from_memory_ptr), "vips_image_new_from_memory") &&
           load_symbol(reinterpret_cast<void**>(&vips_resize_ptr), "vips_resize") &&
           load_symbol(reinterpret_cast<void**>(&vips_image_write_to_memory_ptr), "vips_image_write_to_memory") &&
           load_symbol(reinterpret_cast<void**>(&vips_error_buffer_ptr), "vips_error_buffer") &&
           load_symbol(reinterpret_cast<void**>(&vips_error_clear_ptr), "vips_error_clear") &&
           load_symbol(reinterpret_cast<void**>(&vips_image_get_width_ptr), "vips_image_get_width") &&
           load_symbol(reinterpret_cast<void**>(&vips_image_get_height_ptr), "vips_image_get_height") &&
           load_symbol(reinterpret_cast<void**>(&g_object_unref_ptr), "g_object_unref") &&
           load_symbol(reinterpret_cast<void**>(&g_free_ptr), "g_free");
}

static int kernel_from_algorithm(int algorithmCode) {
    switch (algorithmCode) {
        case 0:
            return VIPS_KERNEL_NEAREST;
        case 1:
            return VIPS_KERNEL_LINEAR;
        case 2:
            return VIPS_KERNEL_CUBIC;
        case 3:
            return VIPS_KERNEL_MITCHELL;
        case 4:
            return VIPS_KERNEL_LANCZOS2;
        case 5:
            return VIPS_KERNEL_LANCZOS3;
        case 6:
            return VIPS_KERNEL_LANCZOS4;
        case 7:
            return VIPS_KERNEL_NOHALO;
        case 8:
            return VIPS_KERNEL_VSHARP;
        case 9:
            return VIPS_KERNEL_VSQBS;
        default:
            return VIPS_KERNEL_LINEAR;
    }
}

static void log_vips_error(const char* context) {
    if (vips_error_buffer_ptr) {
        const char* err = vips_error_buffer_ptr();
        LOGE("%s: %s", context, err ? err : "(unknown)");
    } else {
        LOGE("%s: (vips_error unavailable)", context);
    }
    if (vips_error_clear_ptr) {
        vips_error_clear_ptr();
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_tachiyomi_decoder_VipsWrapper_nativeInit(JNIEnv* env, jclass clazz) {
    if (vips_handle != nullptr) {
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

    if (!ensure_vips_symbols()) {
        LOGE("Failed to resolve required libvips/glib symbols");
        if (vips_shutdown_ptr) {
            vips_shutdown_ptr();
        }
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
    (void)clazz;
    if (!vips_handle || !vips_resize_ptr || !vips_image_new_from_memory_ptr) {
        LOGE("libvips not initialised");
        return nullptr;
    }

    if (inW <= 0 || inH <= 0 || outW <= 0 || outH <= 0 || channels <= 0) {
        LOGE("Invalid dimensions passed to nativeResizeFromMemory");
        return nullptr;
    }

    LOGI("nativeResizeFromMemory %dx%d -> %dx%d, channels=%d, algorithmCode=%d", inW, inH, outW,
         outH, channels, algorithmCode);

    const int64_t input_pixels = static_cast<int64_t>(inW) * static_cast<int64_t>(inH);
    const int64_t expected_len = input_pixels * static_cast<int64_t>(channels);
    if (expected_len <= 0) {
        LOGE("Invalid buffer length calculation");
        return nullptr;
    }

    jsize input_size = env->GetArrayLength(inBuf);
    if (input_size < expected_len) {
        LOGE("Input array too small: have=%d need=%lld", input_size,
             static_cast<long long>(expected_len));
        return nullptr;
    }

    std::vector<uint8_t> input(static_cast<size_t>(expected_len));
    env->GetByteArrayRegion(inBuf, 0, static_cast<jsize>(expected_len),
                            reinterpret_cast<jbyte*>(input.data()));

    VipsImage* in_image =
        vips_image_new_from_memory_ptr(input.data(), static_cast<size_t>(expected_len), inW, inH,
                                       channels, VIPS_FORMAT_UCHAR);
    if (!in_image) {
        log_vips_error("vips_image_new_from_memory");
        return nullptr;
    }

    const double scale_x = static_cast<double>(outW) / static_cast<double>(inW);
    const double scale_y = static_cast<double>(outH) / static_cast<double>(inH);

    if (scale_x <= 0.0 || scale_y <= 0.0) {
        LOGE("Invalid scale factors calculated: %f x %f", scale_x, scale_y);
        g_object_unref_ptr(in_image);
        return nullptr;
    }

    VipsImage* out_image = nullptr;
    const int kernel = kernel_from_algorithm(algorithmCode);
    LOGI("Dispatching to vips_resize with kernel=%d scale=%f x %f", kernel, scale_x, scale_y);

    int resize_status;
    const bool use_vscale = std::fabs(scale_x - scale_y) > 1e-9;
    if (use_vscale) {
        resize_status = vips_resize_ptr(in_image, &out_image, scale_x, "vscale", scale_y, "kernel",
                                        kernel, nullptr);
    } else {
        resize_status =
            vips_resize_ptr(in_image, &out_image, scale_x, "kernel", kernel, nullptr);
    }

    if (resize_status != 0 || !out_image) {
        log_vips_error("vips_resize");
        if (out_image) {
            g_object_unref_ptr(out_image);
        }
        g_object_unref_ptr(in_image);
        return nullptr;
    }

    const int result_w = vips_image_get_width_ptr ? vips_image_get_width_ptr(out_image) : 0;
    const int result_h = vips_image_get_height_ptr ? vips_image_get_height_ptr(out_image) : 0;
    if ((result_w && result_w != outW) || (result_h && result_h != outH)) {
        LOGE("vips_resize produced unexpected size %dx%d (requested %dx%d)", result_w, result_h,
             outW, outH);
    }

    size_t out_size = 0;
    void* out_data = vips_image_write_to_memory_ptr(out_image, &out_size);
    if (!out_data || out_size == 0) {
        log_vips_error("vips_image_write_to_memory");
        if (out_data && g_free_ptr) {
            g_free_ptr(out_data);
        }
        g_object_unref_ptr(out_image);
        g_object_unref_ptr(in_image);
        return nullptr;
    }

    jbyteArray result = env->NewByteArray(static_cast<jsize>(out_size));
    if (!result) {
        LOGE("Failed to allocate output byte array of size %zu", out_size);
        if (g_free_ptr) {
            g_free_ptr(out_data);
        }
        g_object_unref_ptr(out_image);
        g_object_unref_ptr(in_image);
        return nullptr;
    }

    env->SetByteArrayRegion(result, 0, static_cast<jsize>(out_size),
                            reinterpret_cast<jbyte*>(out_data));

    if (g_free_ptr) {
        g_free_ptr(out_data);
    }
    g_object_unref_ptr(out_image);
    g_object_unref_ptr(in_image);

    return result;
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
