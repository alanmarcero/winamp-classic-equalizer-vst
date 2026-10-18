#pragma once

#include "EqConstants.h"
#include "EqSettings.h"
#include "EqSkin.h"

#include <array>
#include <optional>
#include <string>

namespace wce {

struct EqMouseResult {
    bool settingsChanged = false;
    bool openPresetsMenu = false;
    bool windowshadeToggled = false;
    int activeSlider = -1; // -1 if none, 0 = preamp, 1..10 = band
    std::string statusText;
};

// Matches the rendering of Src/Winamp/draw_eq.cpp and the mouse state machine
// of Src/Winamp/Equi.cpp on the 275 x 116 (or 275 x 14 windowshade) EQ window.
class ClassicEqWindow {
public:
    ClassicEqWindow();

    void setCustomSkin(std::optional<SkinBitmap> customMain);
    bool hasCustomSkin() const { return customSkin_.has_value(); }

    bool windowshade() const { return windowshade_; }
    void setWindowshade(bool shaded);

    int currentHeight() const { return windowshade_ ? kShadeHeight : kWindowHeight; }

    // Renders the full EQ window for the given settings and current button/slider press state.
    void render(const EqSettings& settings, bool windowActive = true);

    Rgb pixelAt(int x, int y) const;

    // Mouse event handlers in 1x window coordinates (0..274, 0..115).
    EqMouseResult mouseDown(int x, int y, bool shiftHeld, EqSettings& settings);
    EqMouseResult mouseDrag(int x, int y, bool shiftHeld, EqSettings& settings);
    EqMouseResult mouseUp(int x, int y, bool shiftHeld, EqSettings& settings);
    EqMouseResult doubleClick(int x, int y);

    // Format songticker status string as in do_sliders (Src/Winamp/Equi.cpp):
    // e.g. "EQ: 70Hz: +0.0 dB" or "EQ: PREAMP: +12.0 dB".
    static std::string formatSliderStatus(int which, int pos, FrequencyBands frequencies);

private:
    enum class Capture {
        None,
        TitleButtons,
        QuickButtons,
        ToggleButtons,
        PresetsButton,
        Slider,
    };

    const SkinBitmap& activeMainSkin(FrequencyBands frequencies) const;
    void blitFromSheet(int dstX, int dstY, int w, int h, int srcX, int srcY);
    void blitFromBitmap(const SkinBitmap& src, int dstX, int dstY, int w, int h, int srcX, int srcY);

    void drawTitleBar(bool active);
    void drawTitleButtons(bool closePressed, bool shadePressed);
    void drawPresetsButton(bool pressed);
    void drawOnAutoButtons(bool on, bool autoOn, bool onPressed, bool autoPressed);
    void drawSlider(int which, int pos, bool pressed);
    void drawGraph(int preamp, const BandArray& bands);

    bool handleSliderDrag(int which, int x, int y, bool isMouseDown, bool shiftHeld,
                          EqSettings& settings, EqMouseResult& result);

    SkinBitmap canvas_ {};
    std::optional<SkinBitmap> customSkin_;
    bool windowshade_ = false;

    Capture capture_ = Capture::None;
    int capturedSlider_ = -1;
    int clickYOffset_ = 5;
    bool onPressed_ = false;
    bool autoPressed_ = false;
    bool presetsPressed_ = false;
    bool closePressed_ = false;
    bool shadePressed_ = false;
};

} // namespace wce
