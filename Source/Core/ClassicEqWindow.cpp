#include "ClassicEqWindow.h"

#include "EqGain.h"
#include "Spline.h"

#include <algorithm>
#include <cstdio>

namespace wce {
namespace {

constexpr bool inRegion(int mx, int my, int x1, int y1, int x2, int y2)
{
    return mx >= x1 && mx <= x2 && my >= y1 && my <= y2;
}

constexpr int sliderX(int which)
{
    return (which == 0) ? 21 : 78 + 18 * (which - 1);
}

} // namespace

ClassicEqWindow::ClassicEqWindow()
{
    render(kWinampDefaults, true);
}

void ClassicEqWindow::setCustomSkin(std::optional<SkinBitmap> customMain)
{
    if (customMain && customMain->width >= kWindowWidth && customMain->height >= kWindowHeight)
        customSkin_ = std::move(customMain);
    else
        customSkin_.reset();
}

void ClassicEqWindow::setWindowshade(bool shaded)
{
    windowshade_ = shaded;
}

const SkinBitmap& ClassicEqWindow::activeMainSkin(FrequencyBands frequencies) const
{
    if (customSkin_.has_value())
        return *customSkin_;
    return defaultEqMainSkin(frequencies);
}

void ClassicEqWindow::blitFromSheet(int dstX, int dstY, int w, int h, int srcX, int srcY)
{
    for (int ry = 0; ry < h; ++ry) {
        for (int rx = 0; rx < w; ++rx)
            canvas_.setPixel(dstX + rx, dstY + ry, canvas_.pixelAt(srcX + rx, srcY + ry));
    }
}

void ClassicEqWindow::blitFromBitmap(const SkinBitmap& src, int dstX, int dstY, int w, int h, int srcX, int srcY)
{
    for (int ry = 0; ry < h; ++ry) {
        for (int rx = 0; rx < w; ++rx)
            canvas_.setPixel(dstX + rx, dstY + ry, src.pixelAt(srcX + rx, srcY + ry));
    }
}

void ClassicEqWindow::render(const EqSettings& settings, bool windowActive)
{
    canvas_ = activeMainSkin(settings.frequencies);
    if (canvas_.height < kSkinSheetHeight) {
        // If a custom skin is shorter than 315 rows, fill missing atlas rows from the default skin.
        const auto& fallback = defaultEqMainSkin(settings.frequencies);
        SkinBitmap full = fallback;
        for (int y = 0; y < std::min(canvas_.height, full.height); ++y) {
            for (int x = 0; x < std::min(canvas_.width, full.width); ++x)
                full.setPixel(x, y, canvas_.pixelAt(x, y));
        }
        canvas_ = std::move(full);
    }

    drawSlider(0, settings.preamp, capture_ == Capture::Slider && capturedSlider_ == 0);
    for (int x = 1; x <= kBandCount; ++x) {
        drawSlider(x, settings.bands[static_cast<std::size_t>(x - 1)],
                   capture_ == Capture::Slider && capturedSlider_ == x);
    }
    drawGraph(settings.preamp, settings.bands);
    drawOnAutoButtons(settings.enabled, settings.autoLoad, onPressed_, autoPressed_);
    drawTitleBar(windowActive);
    if (closePressed_ || shadePressed_)
        drawTitleButtons(closePressed_, shadePressed_);
    drawPresetsButton(presetsPressed_);
}

Rgb ClassicEqWindow::pixelAt(int x, int y) const
{
    return canvas_.pixelAt(x, y);
}

// Matches draw_eq_tbar in Src/Winamp/draw_eq.cpp.
void ClassicEqWindow::drawTitleBar(bool active)
{
    if (windowshade_) {
        const auto& ex = defaultEqExSkin();
        blitFromBitmap(ex, 0, 0, kWindowWidth, kShadeHeight, 0, active ? 0 : 15);
        constexpr int kDefaultVolume = 200;
        constexpr int kDefaultPan = 0;
        const int volFrame = (kDefaultVolume * 3) / 256;
        const int volX = 63 + (94 * kDefaultVolume) / 255;
        blitFromBitmap(ex, volX - 2, 4, 3, 7, volFrame * 3 + 1, 30);
        const int panX = 166 + (39 * (kDefaultPan + 128)) / 255;
        const int panFrame = ((kDefaultPan + 128) * 3) / 256;
        blitFromBitmap(ex, panX - 2, 4, 3, 7, panFrame * 3 + 11, 30);
    } else {
        blitFromSheet(0, 0, kWindowWidth, kShadeHeight, 0, active ? 134 : 149);
    }
}

// Matches draw_eq_tbutton in Src/Winamp/draw_eq.cpp.
void ClassicEqWindow::drawTitleButtons(bool closePressed, bool shadePressed)
{
    const auto& ex = defaultEqExSkin();
    if (windowshade_) {
        if (shadePressed)
            blitFromBitmap(ex, 254, 3, 9, 9, 1, 47);
        else
            blitFromBitmap(ex, 254, 3, 9, 9, 254, 3);
        blitFromBitmap(ex, 264, 3, 9, 9, 11, 38 + (closePressed ? 9 : 0));
    } else {
        if (shadePressed && !customSkin_.has_value())
            blitFromBitmap(ex, 254, 3, 9, 9, 1, 38);
        else
            blitFromSheet(254, 3, 9, 9, 254, 137);
        blitFromSheet(264, 3, 9, 9, 0, 116 + (closePressed ? 9 : 0));
    }
}

// Matches draw_eq_presets in Src/Winamp/draw_eq.cpp.
void ClassicEqWindow::drawPresetsButton(bool pressed)
{
    blitFromSheet(217, 18, 44, 12, 224, pressed ? 176 : 164);
}

// Matches draw_eq_onauto in Src/Winamp/draw_eq.cpp.
void ClassicEqWindow::drawOnAutoButtons(bool on, bool autoOn, bool onPressed, bool autoPressed)
{
    blitFromSheet(14, 18, 25, 12, 10 + (onPressed ? 118 : 0) + (on ? 59 : 0), 119);
    blitFromSheet(39, 18, 33, 12, 35 + (autoPressed ? 118 : 0) + (autoOn ? 59 : 0), 119);
}

// Matches draw_eq_slid in Src/Winamp/draw_eq.cpp.
void ClassicEqWindow::drawSlider(int which, int pos, bool pressed)
{
    const int clampedPos = std::clamp(pos, kSliderMin, kSliderMax);
    const int xp = sliderX(which);
    const int n = std::clamp(27 - ((clampedPos * 28) / 64), 0, 27);
    if (n < 14)
        blitFromSheet(xp, 38, 14, 63, 13 + n * 15, 164);
    else
        blitFromSheet(xp, 38, 14, 63, 13 + (n - 14) * 15, 229);

    constexpr int kNumPos = 63 - 11;
    const int thumbY = 38 + 63 - 12 - ((63 - clampedPos) * kNumPos) / 64;
    blitFromSheet(xp + 1, thumbY, 11, 11, 0, pressed ? 176 : 164);
}

// Matches draw_eq_graphthingy in Src/Winamp/draw_eq.cpp.
void ClassicEqWindow::drawGraph(int preamp, const BandArray& bands)
{
    constexpr int kSrcTop = 294;
    constexpr int kLinOffs = 115;

    blitFromSheet(kGraphLeft, kGraphTop, kGraphWidth, kGraphHeight, 0, kSrcTop);
    const int preampRow = std::clamp(preampGraphRow(std::clamp(preamp, kSliderMin, kSliderMax)), 0, kGraphHeight - 1);
    blitFromSheet(kGraphLeft, kGraphTop + preampRow, kGraphWidth, 1, 0, 314);

    const auto spans = computeEqGraphSpans(bands);
    for (int x = 0; x < kSplineColumns; ++x) {
        const auto [topY, bottomY] = spans[static_cast<std::size_t>(x)];
        blitFromSheet(kGraphLeft + 2 + x, kGraphTop + topY, 1, bottomY - topY + 1, kLinOffs, kSrcTop + topY);
    }
}

std::string ClassicEqWindow::formatSliderStatus(int which, int pos, FrequencyBands frequencies)
{
    static constexpr std::array<const char*, kSliderCount> kWinampLabels {
        "PREAMP", "70Hz", "180Hz", "320Hz", "600Hz", "1KHz", "3KHz", "6KHz", "12KHz", "14KHz", "16KHz",
    };
    static constexpr std::array<const char*, kSliderCount> kIsoLabels {
        "PREAMP", "31.5Hz", "63Hz", "125Hz", "250Hz", "500Hz", "1KHz", "2KHz", "4KHz", "8KHz", "16KHz",
    };

    const int idx = std::clamp(which, 0, kSliderCount - 1);
    const char* label = (frequencies == FrequencyBands::Iso)
                            ? kIsoLabels[static_cast<std::size_t>(idx)]
                            : kWinampLabels[static_cast<std::size_t>(idx)];
    const float db = songtickerDb(pos);
    char buffer[64] {};
    std::snprintf(buffer, sizeof(buffer), "EQ: %s: %s%.1f dB", label, db >= 0.0f ? "+" : "", static_cast<double>(db));
    return std::string(buffer);
}

bool ClassicEqWindow::handleSliderDrag(int which, int x, int y, bool isMouseDown, bool shiftHeld,
                                       EqSettings& settings, EqMouseResult& result)
{
    constexpr int kTop = 39;
    constexpr int kBottom = 98;
    constexpr int kWidth = 33 - 21;
    constexpr int kNumPos = 63 - 11;

    const int xoffs = sliderX(which);
    const bool insideThis = inRegion(x, y, xoffs, kTop, xoffs + kWidth, kBottom);
    if ((capture_ != Capture::Slider || capturedSlider_ != which) && !insideThis)
        return false;

    int currentVal = (which == 0) ? settings.preamp : settings.bands[static_cast<std::size_t>(which - 1)];

    // Cross-band sweep check in do_sliders (Src/Winamp/Equi.cpp:215).
    const bool sweptOut = (which > 0)
                          && !(x >= xoffs - 3 && x <= xoffs + kWidth + 3)
                          && (y >= kTop && y <= kBottom && x >= 78 && x <= 180 + 78);

    const int p = 63 - 12 - ((63 - currentVal) * kNumPos) / 64;
    if (isMouseDown) {
        if (y - kTop >= p - 1 && y - kTop < p + 11)
            clickYOffset_ = y - kTop - p;
        else
            clickYOffset_ = 5;
    }

    int d = ((y - clickYOffset_ - kTop) * 64) / kNumPos;
    d = std::clamp(d, kSliderMin, kSliderMax);
    if (!shiftHeld && d >= 30 && d <= 32)
        d = kSliderCenter;

    if (which == 0)
        settings.preamp = d;
    else
        settings.bands[static_cast<std::size_t>(which - 1)] = d;

    result.settingsChanged = true;

    if (sweptOut) {
        capture_ = Capture::None;
        capturedSlider_ = -1;
        return false;
    }

    capture_ = Capture::Slider;
    capturedSlider_ = which;
    result.activeSlider = which;
    result.statusText = formatSliderStatus(which, d, settings.frequencies);
    return true;
}

EqMouseResult ClassicEqWindow::mouseDown(int x, int y, bool shiftHeld, EqSettings& settings)
{
    EqMouseResult result;

    if (inRegion(x, y, 253, 3, 273, 12)) {
        capture_ = Capture::TitleButtons;
        shadePressed_ = (x < 264);
        closePressed_ = (x >= 264);
        render(settings, true);
        return result;
    }

    if (windowshade_)
        return result;

    for (int which = 0; which < kSliderCount; ++which) {
        if (handleSliderDrag(which, x, y, true, shiftHeld, settings, result)) {
            render(settings, true);
            return result;
        }
    }

    if (inRegion(x, y, 14, 18, 14 + 25 + 33, 18 + 12)) {
        capture_ = Capture::ToggleButtons;
        const bool isAuto = (x >= 14 + 25);
        onPressed_ = !isAuto;
        autoPressed_ = isAuto;
        render(settings, true);
        return result;
    }

    if (inRegion(x, y, 217, 18, 217 + 44, 18 + 12)) {
        capture_ = Capture::PresetsButton;
        presetsPressed_ = true;
        render(settings, true);
        return result;
    }

    if (inRegion(x, y, 42, 33, 67, 42) || inRegion(x, y, 42, 65, 67, 74) || inRegion(x, y, 42, 92, 67, 101)) {
        capture_ = Capture::QuickButtons;
        return result;
    }

    return result;
}

EqMouseResult ClassicEqWindow::mouseDrag(int x, int y, bool shiftHeld, EqSettings& settings)
{
    EqMouseResult result;

    switch (capture_) {
    case Capture::TitleButtons: {
        const bool inside = inRegion(x, y, 253, 3, 273, 12);
        shadePressed_ = inside && (x < 264);
        closePressed_ = inside && (x >= 264);
        render(settings, true);
        return result;
    }
    case Capture::ToggleButtons: {
        const bool inside = inRegion(x, y, 14, 18, 14 + 25 + 33, 18 + 12);
        onPressed_ = inside && (x < 14 + 25);
        autoPressed_ = inside && (x >= 14 + 25);
        render(settings, true);
        return result;
    }
    case Capture::PresetsButton: {
        presetsPressed_ = inRegion(x, y, 217, 18, 217 + 44, 18 + 12);
        render(settings, true);
        return result;
    }
    case Capture::QuickButtons:
        return result;
    case Capture::Slider: {
        if (handleSliderDrag(capturedSlider_, x, y, false, shiftHeld, settings, result)) {
            render(settings, true);
            return result;
        }
        // Released due to cross-band sweep: pick up the new band slider under the cursor.
        for (int which = 1; which < kSliderCount; ++which) {
            if (handleSliderDrag(which, x, y, false, shiftHeld, settings, result))
                break;
        }
        render(settings, true);
        return result;
    }
    case Capture::None:
        if (!windowshade_) {
            for (int which = 0; which < kSliderCount; ++which) {
                if (handleSliderDrag(which, x, y, false, shiftHeld, settings, result)) {
                    render(settings, true);
                    return result;
                }
            }
        }
        return result;
    }
    return result;
}

EqMouseResult ClassicEqWindow::mouseUp(int x, int y, bool /*shiftHeld*/, EqSettings& settings)
{
    EqMouseResult result;

    switch (capture_) {
    case Capture::TitleButtons:
        if (inRegion(x, y, 253, 3, 273, 12) && x < 264) {
            windowshade_ = !windowshade_;
            result.windowshadeToggled = true;
        }
        break;
    case Capture::ToggleButtons:
        if (inRegion(x, y, 14, 18, 14 + 25 + 33, 18 + 12)) {
            if (x >= 14 + 25)
                settings.autoLoad = !settings.autoLoad;
            else
                settings.enabled = !settings.enabled;
            result.settingsChanged = true;
        }
        break;
    case Capture::PresetsButton:
        if (inRegion(x, y, 217, 18, 217 + 44, 18 + 12))
            result.openPresetsMenu = true;
        break;
    case Capture::QuickButtons:
        if (inRegion(x, y, 42, 33, 67, 42) || inRegion(x, y, 42, 65, 67, 74) || inRegion(x, y, 42, 92, 67, 101)) {
            const int v = (y <= 42) ? kSliderMin : ((y <= 74) ? kSliderCenter : kSliderMax);
            settings.bands.fill(v);
            result.settingsChanged = true;
        }
        break;
    case Capture::Slider:
    case Capture::None:
        break;
    }

    capture_ = Capture::None;
    capturedSlider_ = -1;
    onPressed_ = false;
    autoPressed_ = false;
    presetsPressed_ = false;
    closePressed_ = false;
    shadePressed_ = false;
    render(settings, true);
    return result;
}

EqMouseResult ClassicEqWindow::doubleClick(int x, int y)
{
    EqMouseResult result;
    if (y <= 14 && x < 252) {
        windowshade_ = !windowshade_;
        result.windowshadeToggled = true;
    }
    return result;
}

} // namespace wce
