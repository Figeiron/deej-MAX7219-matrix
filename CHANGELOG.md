# Changelog: midi-mixer

All notable changes to the MIDI Mixer controller. Tags: `midi-mixer/vX.Y.Z`.

## [1.0.0] - 2026-10-26

First release.

### Firmware
- USB-MIDI firmware for Arduino Pro Micro: sliders on CC1–CC5, buttons on notes 60–65.
- MAX7219 matrix shows the volume MIDI Mixer sends back.
- Smooth cursors: 10-bit resolution, eased jumps, column hysteresis, MIDI echo ignored while a slider moves.
- Device is named "MAX7219 Mixer".

### MIDI Mixer
- Profile with five fader groups (volume feedback to the matrix, mute buttons) and a sixth mute group.
- Volume OSD: EarTrumpet-style overlay with app icon, name and level; follows the Windows theme and accent color.
- Plugin that starts and stops the OSD together with MIDI Mixer.
