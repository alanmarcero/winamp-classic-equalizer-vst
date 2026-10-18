#pragma once

#include "EqConstants.h"
#include "EqSettings.h"

#include <array>

namespace wce {

// eq_lookup1[64] from Src/Winamp/In.cpp. Maps preamp slider position
// 0 (+12.04 dB) through 31 (0 dB, 1.0f) to 63 (-12.04 dB, 0.25f).
inline constexpr std::array<float, 64> kPreampLookup {
    4.000000f, 3.610166f, 3.320019f, 3.088821f, 2.896617f,
    2.732131f, 2.588368f, 2.460685f, 2.345845f, 2.241498f,
    2.145887f, 2.057660f, 1.975760f, 1.899338f, 1.827707f,
    1.760303f, 1.696653f, 1.636363f, 1.579094f, 1.524558f,
    1.472507f, 1.422724f, 1.375019f, 1.329225f, 1.285197f,
    1.242801f, 1.201923f, 1.162456f, 1.124306f, 1.087389f,
    1.051628f, 1.000000f, 0.983296f, 0.950604f, 0.918821f,
    0.887898f, 0.857789f, 0.828454f, 0.799853f, 0.771950f,
    0.744712f, 0.718108f, 0.692110f, 0.666689f, 0.641822f,
    0.617485f, 0.593655f, 0.570311f, 0.547435f, 0.525008f,
    0.503013f, 0.481433f, 0.460253f, 0.439458f, 0.419035f,
    0.398970f, 0.379252f, 0.359868f, 0.340807f, 0.322060f,
    0.303614f, 0.285462f, 0.267593f, 0.250000f,
};

// Linear preamp multiplier from eq_lookup1[preamp].
float preampGain(int pos);

// VALTODB(int v) from Src/Winamp/In.cpp and Src/Winamp/benskiQ/benskiQ.cpp.
// Asymmetric around 31: 31 steps up to +12 dB (0) and 32 steps down to -12 dB (63).
double valToDb(int pos);

// Display dB shown on the main-window songticker while dragging a slider
// in do_sliders (Src/Winamp/Equi.cpp).
float songtickerDb(int pos);

// Bypass condition in eq_set (Src/Winamp/In.cpp): disabled when !on or
// when preamp and all 10 bands are at 31.
bool isBypassed(const EqSettings& settings);

} // namespace wce
