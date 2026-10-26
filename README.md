# deej-MAX7219

Physical audio controllers built on Arduino: sliders and buttons for per-app volume, optionally with a MAX7219 LED matrix. Started as a fork of [omriharel deej](https://github.com/omriharel/deej) with button support from [Miodec deej](https://github.com/Miodec/deej).

![image](assets/build-3d.jpg)

## 📂 Projects

Each project is independent and lives in its own branch, with its own README, changelog, versions and releases.

| Project | Branch | Board | PC software | Releases |
|---|---|---|---|---|
| **MIDI Mixer controller**: matrix shows real app volume, smooth cursors, on-screen volume display | [`midi-mixer`](../../tree/midi-mixer) | Pro Micro | [MIDI Mixer](https://www.midi-mixer.com) + Volume OSD plugin | [`midi-mixer/v*`](../../releases) |
| **deej + MAX7219 matrix**: matrix shows slider positions | [`deej-matrix`](../../tree/deej-matrix) | Nano | deej (this fork) | [`deej-matrix/v*`](../../releases) |
| **deej with buttons (vanilla)**: no display | [`deej-vanilla`](../../tree/deej-vanilla) | Pro Micro | deej (this fork) | [`deej-vanilla/v*`](../../releases) |

## 🏷️ Versions and releases

- Every project has its own [SemVer](https://semver.org) version and a `CHANGELOG.md` in its branch.
- Tags are prefixed with the project name: `midi-mixer/v1.0.0`, `deej-matrix/v1.0.0`, `deej-vanilla/v1.0.0`.
- Pushing a tag makes GitHub Actions on that branch build the firmware and PC software and publish a release with the files.

To release a new version of a project: update its `CHANGELOG.md`, commit, then

```bash
git tag -a midi-mixer/v1.1.0 -m "midi-mixer v1.1.0"
git push origin midi-mixer midi-mixer/v1.1.0
```

## 🧩 Shared hardware

- [`3d_assets`](3d_assets): case models
- [`assets`](assets): photos and the original deej schematic

## 👤 Credits

- Original project: [omriharel](https://github.com/omriharel/deej)
- Button support: [Miodec](https://github.com/Miodec/deej)
- MAX7219 integration, MIDI Mixer controller & this fork: [Figeiron](https://github.com/Figeiron)

## 📄 License

MIT, see [LICENSE](LICENSE).
