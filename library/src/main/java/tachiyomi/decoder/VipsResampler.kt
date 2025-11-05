package tachiyomi.decoder

/** Resampler implementation that delegates to the native VipsWrapper. */
class VipsResampler : Resampler {
    override fun isAvailable(): Boolean = VipsWrapper.isLoaded()

    override fun resize(
        input: ByteArray,
        inW: Int,
        inH: Int,
        channels: Int,
        outW: Int,
        outH: Int,
        algorithmCode: Int
    ): ByteArray? {
        if (!isAvailable()) return null
        return try {
            VipsWrapper.nativeResizeFromMemory(input, inW, inH, channels, outW, outH, algorithmCode)
        } catch (e: UnsatisfiedLinkError) {
            null
        } catch (e: Throwable) {
            null
        }
    }
}
