#include "Spline.h"

#include <algorithm>
#include <cstdint>
#include <ranges>

namespace wce {

float evaluatePlushSpline1D(std::span<const float> keys, float frame,
                            float continuity, float bias, float tension)
{
    const int numKeys = static_cast<int>(keys.size());
    if (numKeys <= 0)
        return 0.0f;

    float a = (1.0f - tension) * (1.0f + continuity) * (1.0f + bias);
    float b = (1.0f - tension) * (1.0f - continuity) * (1.0f - bias);
    float c = (1.0f - tension) * (1.0f - continuity) * (1.0f + bias);
    float d = (1.0f - tension) * (1.0f + continuity) * (1.0f - bias);

    const float t1 = -a / 2.0f;
    const float v1 = t1;
    const float u1 = a;
    const float u2 = (-6.0f - 2.0f * a + 2.0f * b + c) / 2.0f;
    const float v2 = (a - b) / 2.0f;
    const float t2 = (4.0f + a - b - c) / 2.0f;
    const float t3 = (-4.0f + b + c - d) / 2.0f;
    const float u3 = (6.0f - 2.0f * b - c + d) / 2.0f;
    const float v3 = b / 2.0f;
    const float t4 = d / 2.0f;
    const float u4 = -t4;

    const int i0 = static_cast<int>(static_cast<std::uint32_t>(frame));
    int i_1 = i0 - 1;
    while (i_1 < 0)
        i_1 += numKeys;
    int i1 = i0 + 1;
    while (i1 >= numKeys)
        i1 -= numKeys;
    int i2 = i0 + 2;
    while (i2 >= numKeys)
        i2 -= numKeys;

    const float time1 = frame - static_cast<float>(static_cast<std::uint32_t>(frame));
    const float time2 = time1 * time1;
    const float time3 = time2 * time1;

    const float k_1 = keys[static_cast<std::size_t>(i_1)];
    const float k0 = keys[static_cast<std::size_t>(i0)];
    const float k1 = keys[static_cast<std::size_t>(i1)];
    const float k2 = keys[static_cast<std::size_t>(i2)];

    a = t1 * k_1 + t2 * k0 + t3 * k1 + t4 * k2;
    b = u1 * k_1 + u2 * k0 + u3 * k1 + u4 * k2;
    c = v1 * k_1 + v2 * k0 + v3 * k1;
    return a * time3 + b * time2 + c * time1 + k0;
}

std::array<int, kSplineColumns> computeEqGraphPoints(const BandArray& bands)
{
    std::array<float, 12> keys {};
    for (int x = 0; x < kBandCount; ++x)
        keys[static_cast<std::size_t>(x + 1)] = static_cast<float>(bands[static_cast<std::size_t>(x)]) * 19.0f / 64.0f;
    keys[0] = keys[1];
    keys[11] = keys[10];

    std::array<int, kSplineColumns> points {};
    for (int x = 0; x < kSplineColumns; ++x) {
        const float p = evaluatePlushSpline1D(keys, 1.0f + static_cast<float>(x) / 12.0f, 0.0f, 0.0f, 0.1f);
        int thisP = static_cast<int>(p);
        if (thisP < 0)
            thisP = 0;
        if (thisP > 18)
            thisP = 18;
        points[static_cast<std::size_t>(x)] = thisP;
    }
    return points;
}

std::array<SplineSpan, kSplineColumns> computeEqGraphSpans(const BandArray& bands)
{
    const auto points = computeEqGraphPoints(bands);
    std::array<SplineSpan, kSplineColumns> spans {};
    int lastP = -1;
    for (int x = 0; x < kSplineColumns; ++x) {
        const int thisP = points[static_cast<std::size_t>(x)];
        if (lastP == -1 || thisP == lastP) {
            spans[static_cast<std::size_t>(x)] = { thisP, thisP };
        } else if (thisP < lastP) {
            spans[static_cast<std::size_t>(x)] = { thisP, lastP };
        } else {
            spans[static_cast<std::size_t>(x)] = { lastP, thisP };
        }
        lastP = thisP;
    }
    return spans;
}

int preampGraphRow(int preamp)
{
    return kGraphHeight - 1 - static_cast<int>(static_cast<float>(preamp) * 19.0f / 64.0f);
}

} // namespace wce
