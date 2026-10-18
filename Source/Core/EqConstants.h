#pragma once

namespace wce {

inline constexpr int kWindowWidth = 275;
inline constexpr int kWindowHeight = 116;
inline constexpr int kShadeHeight = 14;

inline constexpr int kSkinSheetWidth = 275;
inline constexpr int kSkinSheetHeight = 315;
inline constexpr int kExSheetWidth = 275;
inline constexpr int kExSheetHeight = 82;

inline constexpr int kBandCount = 10;
inline constexpr int kSliderCount = 11; // 0 = preamp, 1..10 = bands

// Slider positions in Src/Winamp/Eq.cpp and Src/Winamp/In.cpp:
// 0 is the top (+12 dB), 31 is flat (0 dB), 63 is the bottom (-12 dB).
inline constexpr int kSliderMin = 0;
inline constexpr int kSliderCenter = 31;
inline constexpr int kSliderMax = 63;

inline constexpr int kGraphLeft = 86;
inline constexpr int kGraphTop = 17;
inline constexpr int kGraphWidth = 113;
inline constexpr int kGraphHeight = 19;
inline constexpr int kSplineColumns = 109;

} // namespace wce
