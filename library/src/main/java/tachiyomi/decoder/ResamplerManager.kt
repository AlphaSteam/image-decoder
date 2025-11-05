package tachiyomi.decoder

/**
 * Simple manager that chooses an available Resampler implementation.
 * By default it prefers the native VipsResampler when available, falls back to NoopResampler.
 */
object ResamplerManager {
    private val impl: Resampler

    init {
        impl = if (VipsWrapper.isLoaded()) {
            try {
                VipsResampler()
            } catch (e: Throwable) {
                NoopResampler()
            }
        } else {
            NoopResampler()
        }
    }

    fun get(): Resampler = impl
}
