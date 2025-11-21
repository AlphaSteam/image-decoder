package tachiyomi.decoder

import android.util.Log

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
            val algorithmName = ScalingAlgorithm.fromCode(algorithmCode)?.name ?: "code=$algorithmCode"
            Log.d(
                TAG,
                "libvips resize ${inW}x${inH} -> ${outW}x${outH}, channels=$channels, algorithm=$algorithmName"
            )
            VipsWrapper.nativeResizeFromMemory(input, inW, inH, channels, outW, outH, algorithmCode)
        } catch (e: UnsatisfiedLinkError) {
            Log.w(TAG, "libvips resize failed – UnsatisfiedLinkError for algorithmCode=$algorithmCode", e)
            null
        } catch (e: Throwable) {
            Log.w(TAG, "libvips resize threw for ${inW}x${inH} -> ${outW}x${outH}", e)
            null
        }
    }

    private companion object {
        const val TAG = "VipsResampler"
    }
}
