package tachiyomi.decoder

enum class ScalingAlgorithm(val code: Int){
    NEAREST_NEIGHBOR(code = 0),
    BILINEAR(code = 1);

    companion object {
        private val codeMap = entries.associateBy { it.code }
            @JvmStatic
            fun fromCode(code: Int): ScalingAlgorithm? = codeMap[code]

            @JvmStatic
            fun isValidCode(code: Int): Boolean = code in codeMap
    }
}