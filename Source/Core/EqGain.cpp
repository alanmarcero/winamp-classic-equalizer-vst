#include "EqGain.h"

#include <algorithm>

namespace wce {

float preampGain(int pos)
{
    return kPreampLookup[static_cast<std::size_t>(std::clamp(pos, kSliderMin, kSliderMax))];
}

double valToDb(int pos)
{
    int v = pos - kSliderCenter;
    if (v < -31)
        v = -31;
    if (v > 32)
        v = 32;

    if (v > 0)
        return -12.0 * (v / 32.0);
    if (v < 0)
        return -12.0 * (v / 31.0);
    return 0.0;
}

float songtickerDb(int pos)
{
    const int d = std::clamp(pos, kSliderMin, kSliderMax);
    float v = static_cast<float>(d);
    v -= 31.5f;
    v /= 31.5f;
    v *= -12.0f;
    if (v >= -0.32f && v <= 0.32f)
        v = 0.0f;
    return v;
}

bool isBypassed(const EqSettings& settings)
{
    if (!settings.enabled)
        return true;
    if (settings.preamp != kSliderCenter)
        return false;
    return std::ranges::all_of(settings.bands, [](int band) { return band == kSliderCenter; });
}

} // namespace wce
