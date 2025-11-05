package tachiyomi.decoder

/**
 * Resampler abstraction. Implementations may use native libraries (libvips) or be a no-op.
 */
interface Resampler {
    fun isAvailable(): Boolean

    /**
     * Resize an interleaved u8 pixel buffer.
     * @param input input pixels (length == inW*inH*channels)
     * @param inW input width
     * @param inH input height
     * @param channels number of channels (usually 4)
     * @param outW desired output width
     * @param outH desired output height
     * @param algorithmCode integer code describing the desired scaling algorithm (e.g. 0=nearest,1=bilinear)
     *                      Codes are defined in the shared `ScalingAlgorithm` enum in the view module.
     * @return resized pixel buffer (length == outW*outH*channels) or null on failure
     */
    fun resize(
        input: ByteArray,
        inW: Int,
        inH: Int,
        channels: Int,
        outW: Int,
        outH: Int,
        algorithmCode: Int = 1
    ): ByteArray?
}
