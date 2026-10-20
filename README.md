# Winamp Classic Equalizer (VST3)

The 10-band graphic equalizer from Winamp's classic Equalizer window, as a VST3 effect. It reproduces both the 275 x 116 classic Equalizer interface (including the Plush spline response graph, Windowshade mode, custom `eqmain.bmp` skin loading, and `.eqf` preset files) and both of Winamp's EQ DSP engines (**Winamp 4Front EQ** and **Constant-Q EQ**).

## About

This plugin is inspired by Winamp. It is a complete redevelopment and does not redistribute any Winamp code. The Winamp source release was used only as a reference for how the classic equalizer behaves.

## Source reference

The reference is the Winamp source release, as mirrored at [alexfreud/winamp](https://github.com/alexfreud/winamp) (branch `community`, commit [`0954e03`](https://github.com/alexfreud/winamp/tree/0954e03b3acbc11b6c90298598db6b6469568d72)). These are the files used as reference:

| Original file | What it holds | Implemented in |
|---|---|---|
| [`Src/Winamp/eq10dsp.cpp`](https://github.com/alexfreud/winamp/blob/0954e03b3acbc11b6c90298598db6b6469568d72/Src/Winamp/eq10dsp.cpp), [`eq10dsp.h`](https://github.com/alexfreud/winamp/blob/0954e03b3acbc11b6c90298598db6b6469568d72/Src/Winamp/eq10dsp.h) | George Yohng's 4Front EQ10 resonator bank (`Q = 1.41`, boost `Q * 2`, cut `Q * 0.5`) and peak limiter (`0.930` threshold, `700 ms` release) | `Source/Core/FourFrontEq.*` |
| [`Src/Winamp/benskiQ/benskiQ.cpp`](https://github.com/alexfreud/winamp/blob/0954e03b3acbc11b6c90298598db6b6469568d72/Src/Winamp/benskiQ/benskiQ.cpp), [`EqBand.cpp`](https://github.com/alexfreud/winamp/blob/0954e03b3acbc11b6c90298598db6b6469568d72/Src/Winamp/benskiQ/EqBand.cpp), [`Biquad.cpp`](https://github.com/alexfreud/winamp/blob/0954e03b3acbc11b6c90298598db6b6469568d72/Src/Winamp/benskiQ/Biquad.cpp) | Ben Allison's Constant-Q peaking biquad EQ (`Q = 1.41`, bilinear `s`-to-`z` transform, pair-unrolled Direct Form I) | `Source/Core/ConstantQEq.*` |
| [`Src/Winamp/In.cpp`](https://github.com/alexfreud/winamp/blob/0954e03b3acbc11b6c90298598db6b6469568d72/Src/Winamp/In.cpp) | `eq_lookup1[64]` preamp table, `VALTODB` asymmetric 64-step dB mapping, `eq_set` bypass rules, `eq_dosamples` dispatch | `Source/Core/EqGain.*`, `Source/Core/EqualizerDsp.*` |
| [`Src/Winamp/draw_eq.cpp`](https://github.com/alexfreud/winamp/blob/0954e03b3acbc11b6c90298598db6b6469568d72/Src/Winamp/draw_eq.cpp) | `draw_eq_slid` (28 LED track frames + thumb), `draw_eq_graphthingy` (113 x 19 spline box), `draw_eq_onauto`, `draw_eq_presets`, `draw_eq_tbar` | `Source/Core/ClassicEqWindow.*`, `Source/Core/EqSkin.*` |
| [`Src/Winamp/plush/SPLINE.C`](https://github.com/alexfreud/winamp/blob/0954e03b3acbc11b6c90298598db6b6469568d72/Src/Winamp/plush/SPLINE.C) | `plSplineGetPoint` (12-key Kochanek-Bartels spline with `tension = 0.1` across 109 columns) | `Source/Core/Spline.*` |
| [`Src/Winamp/Equi.cpp`](https://github.com/alexfreud/winamp/blob/0954e03b3acbc11b6c90298598db6b6469568d72/Src/Winamp/Equi.cpp) | Slider hit-testing, center-snap (`30..32 -> 31`), cross-band drag sweep, `+12 dB` / `+0 dB` / `-12 dB` quick buttons, songticker readout | `Source/Core/ClassicEqWindow.*` |
| [`Src/Winamp/Eq.cpp`](https://github.com/alexfreud/winamp/blob/0954e03b3acbc11b6c90298598db6b6469568d72/Src/Winamp/Eq.cpp), [`Src/Winamp/main.cpp`](https://github.com/alexfreud/winamp/blob/0954e03b3acbc11b6c90298598db6b6469568d72/Src/Winamp/main.cpp) | `CreateEQPresets` (18 built-in presets) and `.eqf` / `Winamp.q1` binary preset reader and writer | `Source/Core/EqPresets.*` |
| [`Src/Winamp/WinampAttributes.cpp`](https://github.com/alexfreud/winamp/blob/0954e03b3acbc11b6c90298598db6b6469568d72/Src/Winamp/WinampAttributes.cpp), [`options_playback.cpp`](https://github.com/alexfreud/winamp/blob/0954e03b3acbc11b6c90298598db6b6469568d72/Src/Winamp/options_playback.cpp) | Equalizer preferences (`config_eq_type`, `config_eq_frequencies`, `config_eq_limiter`) | `Source/Core/EqSettings.h`, `Source/Plugin/Parameters.*` |

## What it reproduces

- **4Front EQ10 (default)**: 10 cascaded resonator bands with base `Q = 1.41` and asymmetric bandwidths (`Q * 2.0 = 2.82` for boost, `Q * 0.5 = 0.705` for cut), where each band adds `(in[0] - x2) * a0 + y1 * b1 + y2 * b2 + 1e-30` to the dry signal with `gain = 10^(dB / 20) - 1`. Followed by the per-channel dynamic peak limiter (`EQ10_TRIM_CODE = 0.930`, `EQ10_TRIM_RELEASE = 0.700 s`).
- **Constant-Q EQ (`benskiQ`)**: 10 cascaded peaking biquad filters (`Q = 1.41`) derived from an analog `s`-domain prototype via bilinear transform with `tan(pi * f0 / fs)` pre-warping, processed in `benskiQ`'s pair-unrolled Direct Form I loop.
- **Frequency tables**: both **Winamp frequency bands** (`70, 180, 320, 600, 1000, 3000, 6000, 12000, 14000, 16000 Hz`) and **ISO Standard frequency bands** (`31/31.5, 62/63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000 Hz`), switching the window's band labels to `eqmain_iso.bmp` when ISO is selected.
- **64-step sliders and preamp**: every slider steps through Winamp's `0..63` integer positions (`0` = +12 dB, `31` = 0 dB, `63` = -12 dB). Preamp uses the 64-entry `eq_lookup1` table (`4.0` down to `0.25`, with `1.0` at `31`), and the 10 bands use `VALTODB`'s asymmetric dB mapping (`-12 * v / 31` for boost, `-12 * v / 32` for cut). When **ON** is off or all 11 sliders sit at `31`, the DSP bypasses completely.
- **Spline graph & UI interaction**: `draw_eq_graphthingy` evaluates Justin Frankel's Plush 1.2 spline (`tension = 0.1`) across 109 columns over the 113 x 19 graph box and colors each vertical span from the 19-row gradient strip alongside the preamp line. Dragging horizontally across the 10 bands sweeps them in a single stroke, sliders snap to `0 dB` (`30..32 -> 31`) unless `Shift` is held, and clicking the `+12 dB`, `+0 dB`, or `-12 dB` labels sets all 10 bands at once.

### One deliberate difference

Winamp's `eq_set` in `Src/Winamp/In.cpp` (line 1264) scans for the last non-flat band with `for ( x = 9; x >= 0 && data[x] == 31; x++ )`. It counts up instead of down. When the 16 kHz slider is not flat, `filter_top2` becomes `9`, so Winamp's Constant-Q engine never processes the 16 kHz band. When that slider is flat, the loop reads past the end of the array. This plugin does not reproduce that bug: all 10 Constant-Q bands follow their sliders. The 4Front engine does not use `filter_top2`, so it matches Winamp in both cases.

## Options

Select **PRESETS** on the equalizer window for Winamp's preset menu, or select **Settings** (or right-click anywhere on the window) for the Equalizer options menu. Double-click the titlebar or select the titlebar button to toggle Windowshade mode.

| Option | Values | Default |
|---|---|---|
| Equalizer enabled (ON) | on, off | on |
| Auto-load (AUTO) | on, off | off |
| EQ type | Winamp 4Front EQ, Constant-Q EQ | Winamp 4Front EQ |
| Frequency bands | Winamp frequency bands, ISO Standard frequency bands | Winamp frequency bands |
| Use limiter | on, off | on |
| Preamp & Bands 1-10 | -12.0 dB to +12.0 dB (64 Winamp steps) | +0.0 dB (step 31) |
| Presets | 18 built-in Winamp presets, Default, Load/Save `.eqf` / `Winamp.q1` | Flat |
| Skin | Load a skin's `eqmain.bmp`, or the default classic skin | default |
| Windowshade mode | Normal (275 x 116), Shaded (275 x 14) | Normal |
| Size | 1x to 5x | 2x |

Every audio option (ON, AUTO, EQ type, Frequency bands, Use limiter, Preamp, and all 10 bands) is a host parameter, so it saves with the project and can be automated.

## Build

GitHub Actions builds a universal macOS VST3 and a 64-bit Windows VST3. On each platform it runs the unit tests and `pluginval`, then uploads the zipped bundle as an artifact. To build by hand:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The unit tests cover the core in `Source/Core` and need no JUCE.

## Install

Download the artifact for your platform from the latest run on the repo's Actions page. GitHub wraps each artifact in a zip, so unzip it once to get the plugin's zip, then unzip that.

### macOS

Move `Winamp Classic Equalizer.vst3` into `~/Library/Audio/Plug-Ins/VST3/`. The bundle is signed ad hoc, so clear the quarantine flag once:

```sh
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Winamp Classic Equalizer.vst3"
```

### Windows

1. Right-click `WinampClassicEqualizer-VST3-Windows.zip`, select **Properties**, check **Unblock**, and select **OK**. Do this before you unzip, so the extracted files are not marked as downloaded.
2. Unzip it. You get a folder named `Winamp Classic Equalizer.vst3`.
3. Move that folder into `C:\Program Files\Common Files\VST3\`. Windows asks for administrator permission.
4. Rescan plugins in your DAW.

Only 64-bit DAWs can load it.
