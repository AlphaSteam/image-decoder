package tachiyomi.decoder

import android.util.Log

/**
 * Simple manager that chooses an available Resampler implementation.
 * By default it prefers the native VipsResampler when available, falls back to NoopResampler.
 */
object ResamplerManager {
    private val impl: Resampler

    init {
        impl = if (VipsWrapper.isLoaded()) {
            try {
                Log.i(TAG, "libvips detected – using VipsResampler for tile scaling.")
                VipsResampler()
            } catch (e: Throwable) {
                Log.w(TAG, "Failed to initialise VipsResampler, falling back to no-op.", e)
                NoopResampler()
            }
        } else {
            Log.i(TAG, "libvips unavailable – falling back to no-op resampler.")
            NoopResampler()
        }
    }

    fun get(): Resampler = impl

    private const val TAG = "ResamplerManager"
}
