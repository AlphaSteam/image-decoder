package tachiyomi.decoder

enum class ScalingAlgorithm(val code: Int){
    ORIGINAL(code = -1),
    NEAREST_NEIGHBOR(code = 0),
    BILINEAR(code = 1),
    BICUBIC(code = 2),
    MITCHELL(code = 3),
    LANCZOS2(code = 4),
    LANCZOS3(code = 5),
    LANCZOS4(code = 6),
    NOHALO(code = 7),
    VSHARP(code = 8),
    VSQBS(code = 9);
    

    companion object {
        private val codeMap = entries.associateBy { it.code }

        /**
         * Default algorithm exported for cross-module defaults.
         * Use `ScalingAlgorithm.DEFAULT` when you want the canonical default enum value.
         */
        @JvmField
        val DEFAULT: ScalingAlgorithm = BILINEAR

        /**
         * Default algorithm code exported for JNI / integer-based APIs.
         */
        @JvmField
        val DEFAULT_CODE: Int = DEFAULT.code

        @JvmStatic
        fun fromCode(code: Int): ScalingAlgorithm? = codeMap[code]

        @JvmStatic
        fun isValidCode(code: Int): Boolean = code in codeMap
    }
}