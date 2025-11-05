package tachiyomi.decoder

/**
 * JNI wrapper around native libvips-based functions.
 * This class will attempt to load the native wrapper library; if it fails the methods
 * will indicate the library is not available.
 */
object VipsWrapper {
    private var loaded = false

    init {
        try {
            System.loadLibrary("vipswrapper")
            loaded = nativeInit()
        } catch (e: UnsatisfiedLinkError) {
            loaded = false
        } catch (e: Throwable) {
            loaded = false
        }
    }

    /** Returns true if the native wrapper was loaded and init succeeded. */
    fun isLoaded(): Boolean = loaded

    @JvmStatic
    private external fun nativeInit(): Boolean

    @JvmStatic
    external fun nativeResizeFromMemory(
        input: ByteArray,
        inW: Int,
        inH: Int,
        channels: Int,
        outW: Int,
        outH: Int,
        algorithmCode: Int
    ): ByteArray?

    @JvmStatic
    external fun nativeShutdown()
}
