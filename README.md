# deej with buttons (vanilla)

A fork of [omriharel deej](https://github.com/omriharel/deej) with **push button support** (from [Miodec deej](https://github.com/Miodec/deej)), on an Arduino Pro Micro without a display.

> This is the `deej-vanilla` branch. Other projects for the same hardware live in their own branches: see [`master`](../../tree/master).

## 📦 Features

- 🎚️ 5 sliders control application volumes
- 🔘 6 buttons for actions (mute, pause, hotkeys...)
- All original deej features preserved

## 🔌 Wiring (Pro Micro)

| Part | Pin |
|---|---|
| Sliders 1–5 (wiper) | A3, A2, A1, A0, 10 |
| Buttons 1–6 (to GND) | 9, 8, 7, 6, 5, 4 |
| Slider ends | VCC and GND |

## 🧠 Software Setup

1. Download the latest [`deej-vanilla/v*` release](../../releases).
2. Flash `deej-vanilla-firmware.hex`, or open `arduino/deej-5-sliders-vanilla` in the Arduino IDE and upload it (board: Arduino Leonardo or SparkFun Pro Micro).
3. Put `deej.exe` and `config.yaml` in one folder, set up `config.yaml` as in original deej and run `deej.exe`.

## 🏷️ Releases

Versions follow [SemVer](https://semver.org); changes are listed in [CHANGELOG.md](CHANGELOG.md). Pushing a tag like `deej-vanilla/v1.2.0` makes GitHub Actions build the firmware and `deej.exe` and publish them as a release.

## 👤 Credits

- Original project: [omriharel](https://github.com/omriharel/deej)
- Button support: [Miodec](https://github.com/Miodec/deej)
- This fork: [Figeiron](https://github.com/Figeiron)

## 📄 License

MIT, see [LICENSE](LICENSE).
