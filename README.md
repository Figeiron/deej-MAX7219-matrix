# deej-MAX7219

A fork of [omriharel deej](https://github.com/omriharel/deej) with additional features:

- ✅ **Support for push buttons** (from [Miodec deej](https://github.com/Miodec/deej))
- ✅ **MAX7219 LED matrix display support** (shows current potentiometers values)
- ✅ All original deej features preserved

> This is the `deej-matrix` branch (Arduino Nano + MAX7219 matrix + deej). Other projects for the same hardware live in their own branches: see [`master`](../../tree/master).

![image](assets/build-3d.jpg)  

## 💡 About

This project allows you to build a physical audio controller using Arduino-compatible boards.  
This fork extends the original deej with support for:

- Tactile buttons (e.g., to mute apps or run hotkeys)
- Visual display via MAX7219 matrix
- Full compatibility with Miodec's original deej logic

![connection diagram](assets/schematic.png)

---

## 📦 Features

- 🎚️ Analog potentiometers to control application volumes
- ⬛ MAX7219 LED matrix shows real-time values
- 🔘 Customizable buttons for actions (mute, pause, etc.)

---

## ⚙️ Hardware Requirements

- Arduino 
- MAX7219 LED matrix
- Potentiometers or linear sliders
- Optional: push buttons
- Optional: capacitors for voltage stability

---

## 🔌 Wiring Notes

- Potentiometers: Connect to analog pins
- Buttons: Connect to digital pins 
- MAX7219: Connect via SPI

---

## 🧠 Software Setup

1. Download the latest [`deej-matrix/v*` release](../../releases).
2. Flash `deej-matrix-firmware.hex` to the Nano, or build from source with `pio run -t upload` in `arduino/deej-5-sliders-matrix-pio`.
3. Put `deej.exe` and `config.yaml` in one folder, set up `config.yaml` as in original deej and run `deej.exe`.
4. Enjoy physical control over your system audio!

---

## 🏷️ Releases

Versions follow [SemVer](https://semver.org); changes are listed in [CHANGELOG.md](CHANGELOG.md). Pushing a tag like `deej-matrix/v1.2.0` makes GitHub Actions build the firmware and `deej.exe` and publish them as a release.

---

## 👤 Credits

- Original project: [omriharel](https://github.com/omriharel/deej)
- Button support: [Miodec](https://github.com/Miodec/deej)
- MAX7219 integration & this fork: [Figeiron](https://github.com/Figeiron)

## 📄 License

This project follows the original [MIT License](https://github.com/omriharel/deej/blob/master/LICENSE).

