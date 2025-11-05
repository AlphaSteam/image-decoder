package tachiyomi.decoder

/** Simple pass-through resampler used when no native resampler is available. */
class NoopResampler : Resampler {
    override fun isAvailable(): Boolean = true

    override fun resize(
        input: ByteArray,
        inW: Int,
        inH: Int,
        channels: Int,
        outW: Int,
        outH: Int,
        algorithmCode: Int
    ): ByteArray? {
        // If requested output matches input size, return the input buffer directly.
        if (inW == outW && inH == outH) return input

        // Otherwise, we don't implement a Java resizer here — return null so callers
        // can fall back to the decoder's bitmap creation path.
        return null
    }
}
