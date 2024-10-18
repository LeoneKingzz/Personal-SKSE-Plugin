#pragma once

#include "RE/T/TESTexture.h"
#include "RE/C/Color.h"

namespace RE
{
    class TintMask
    {
    public:
        enum class Type
        {
            kFrekles = 0,
            kLips,
            kCheeks,
            kEyeliner,
            kUpperEyeSocket,
            kLowerEyeSocket,
            kSkinTone,
            kWarPaint,
            kFrownLines,
            kLowerCheeks,
            kNose,
            kChin,
            kNeck,
            kForehead,
            kDirt,

            kTotal
        };

        // members
        TESTexture*                           texture;
        Color                                 color;
        float                                 alpha;
        stl::enumeration<Type, std::uint32_t> type;
    };
    static_assert(sizeof(TintMask) == 0x18);
}
