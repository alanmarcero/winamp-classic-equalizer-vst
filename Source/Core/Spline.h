#pragma once

#include "EqConstants.h"
#include "EqSettings.h"

#include <array>
#include <span>
#include <utility>

namespace wce {

struct SplineSpan {
    int topY = 0;    // Inclusive row in [0, 18]
    int bottomY = 0; // Inclusive row in [0, 18]
};

// Matches plSplineGetPoint in Src/Winamp/plush/SPLINE.C (Plush 1.2)
// and the spline curve loop in draw_eq_graphthingy (Src/Winamp/draw_eq.cpp).
float evaluatePlushSpline1D(std::span<const float> keys, float frame,
                            float continuity = 0.0f, float bias = 0.0f, float tension = 0.1f);

// Returns the 109 clamped Y positions (0..18) for x = 0..108 across the 10 bands.
std::array<int, kSplineColumns> computeEqGraphPoints(const BandArray& bands);

// Returns the 109 vertical spans [topY, bottomY] drawn at left + 2 + x in draw_eq_graphthingy.
std::array<SplineSpan, kSplineColumns> computeEqGraphSpans(const BandArray& bands);

// Preamp line Y offset (0..18) inside the 113 x 19 graph box:
// top - 1 + h - (int)(config_preamp * 19.0f / 64.0f) relative to top.
int preampGraphRow(int preamp);

} // namespace wce
