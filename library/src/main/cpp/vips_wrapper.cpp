// Minimal JNI stub for Vips wrapper. Returns failures so code can run without libvips.
#include <jni.h>

extern "C" JNIEXPORT jboolean JNICALL
Java_tachiyomi_decoder_VipsWrapper_nativeInit(JNIEnv* env, jclass clazz) {
    // Native lib not present in stub. Return false to indicate not initialised.
    return JNI_FALSE;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_tachiyomi_decoder_VipsWrapper_nativeResizeFromMemory(JNIEnv* env, jclass clazz,
                                                          jbyteArray inBuf, jint inW, jint inH,
                                                          jint channels, jint outW, jint outH,
                                                          jint algorithmCode) {
    // Stub: return null to indicate resize not performed.
    (void)inBuf; (void)inW; (void)inH; (void)channels; (void)outW; (void)outH; (void)algorithmCode;
    return nullptr;
}

extern "C" JNIEXPORT void JNICALL
Java_tachiyomi_decoder_VipsWrapper_nativeShutdown(JNIEnv* env, jclass clazz) {
    // No-op stub
}
