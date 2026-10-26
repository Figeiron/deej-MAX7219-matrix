# MAX7219 MIDI Mixer controller

A 5-slider, 6-button audio controller on an **Arduino Pro Micro** with a **4×8×8 MAX7219 LED matrix**, working as a USB-MIDI device for [MIDI Mixer](https://www.midi-mixer.com).

> This is the `midi-mixer` branch. Other projects for the same hardware live in their own branches: see [`master`](../../tree/master).

![image](assets/build-3d.jpg)

## What's inside

| Folder | What it is |
|---|---|
| [`arduino/midi-mixer-5-sliders-matrix-pio`](arduino/midi-mixer-5-sliders-matrix-pio) | PlatformIO firmware for the Pro Micro |
| [`midi-mixer-profile`](midi-mixer-profile) | MIDI Mixer profile for the controller |
| [`volume-osd`](volume-osd) | EarTrumpet-style on-screen volume display (C#/WPF) |
| [`midi-mixer-plugin`](midi-mixer-plugin) | MIDI Mixer plugin that starts the OSD together with MIDI Mixer |
| [`3d_assets`](3d_assets) | Case models |

## Features

- **5 sliders** send CC1–CC5, **6 buttons** send notes 60–65 (channel 1).
- **The matrix shows the real volume** that MIDI Mixer sends back, so changes made in Windows show up too. Slider 1 (master) is a full-height line, the others are 2-row cursors.
- **Smooth cursors**: full 10-bit pot resolution, eased jumps, no jitter on column edges.
- Shows up in Windows as **MAX7219 Mixer**, no drivers needed.

## Wiring (Pro Micro)

| Part | Pro Micro pin |
|---|---|
| Matrix DIN / CLK / CS | 16 / 15 / 3 |
| Sliders 1–5 (wiper) | A0, A1, A2, A3, 10 |
| Buttons 1–6 (to GND) | 9, 8, 7, 6, 5, 4 |
| Matrix VCC, slider ends | VCC and GND |

## Install

Download the files from the latest [`midi-mixer/v*` release](../../releases):

1. **Firmware**: flash `midi-mixer-firmware.hex`, or build from source with `pio run -t upload` in `arduino/midi-mixer-5-sliders-matrix-pio`.
2. **Profile**: unzip `midi-mixer-profile.zip` into `%APPDATA%\midi-mixer-app\profiles\`, restart MIDI Mixer and load *deej MAX7219 (Pro Micro)*.
3. **OSD (optional)**: unzip `volume-osd-plugin.zip` into `%APPDATA%\midi-mixer-app\plugins\`, refresh plugins in MIDI Mixer and enable *Volume OSD*. Needs the [.NET 10 Desktop Runtime](https://dotnet.microsoft.com/download/dotnet/10.0). Turn off MIDI Mixer's own OSD in Settings → OSD.

To build and install the OSD from source: `powershell -ExecutionPolicy Bypass -File midi-mixer-plugin/install.ps1`.

## Releases

Versions follow [SemVer](https://semver.org); changes are listed in [CHANGELOG.md](CHANGELOG.md). Pushing a tag like `midi-mixer/v1.2.0` makes GitHub Actions build the firmware, profile and OSD and publish them as a release.

## License

MIT, see [LICENSE](LICENSE).
