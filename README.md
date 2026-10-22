# NyanCat Tandy for DOS

A Tandy 1000/PCjr showcase build of [NyanCat-XT](https://github.com/OlivierP-To8/NyanCat-XT), originally written by Olivier P-To8. It keeps the original DOS animation and Sound Blaster path, and adds a 16-color Tandy renderer with a larger, crisp 3x pixel-art animation and a score-based PSG arrangement.

## Download

- [`dist/NyanCat-Tandy-8088-v1.0.0.zip`](dist/NyanCat-Tandy-8088-v1.0.0.zip) - playable package with the DOS executable, instructions, and required notices.
- [`dist/NYANCAT.EXE`](dist/NYANCAT.EXE) - standalone executable.
- [`dist/SHA256SUMS.txt`](dist/SHA256SUMS.txt) - SHA-256 checksums.

The executable is 88,768 bytes. SHA-256: `AD8219CE2663910A0600E1D15F56CBDEED9659CC40F6EA58805572E3C60DD344`.

## Requirements and modes

- **Tandy 1000 or PCjr-compatible system:** run `NYANCAT.EXE /tandy`. This selects 320x200 16-color graphics and PSG audio; it does not require a Sound Blaster.
- **Original XT/CGA or EGA path:** the original video and Sound Blaster WAV playback remain the default. Set the `BLASTER` environment variable for your card. Use `/wav:filename.wav` to select another WAV file.
- `/tandy1x` keeps the original-size Tandy animation; the default `/tandy` view uses crisp 3x pixel replication.
- `/fps:1` through `/fps:30` selects the animation frame rate (default 15). The Tandy music scheduler runs at 60 Hz independently of the requested video rate.
- The original `/dsp:` and `/dma:` options remain available for the Sound Blaster path.

The Tandy renderer is custom application code. LibTandy is used for score scheduling and PSG/audio output only; it does not provide graphics. The score plays the MIDI-derived intro once and then repeats the complete A/B phrase. It uses all three PSG tone channels, noise percussion, and short PC-speaker kick accents when the speaker gate is available. The Tandy mode can run without a Sound Blaster.

## Build

Build with Open Watcom C16 1.9 installed and on `PATH`:

```text
wmake -f MAKEFILE.WCC
```

This produces `NYANCAT.EXE` in the project root. The Tandy path is DOS 8088/8086-compatible and uses no runtime DLL or separate LibTandy installation; the required LibTandy subset is vendored under `third_party/libtandy`.

## Validation

The published executable was run in DOSBox-X using `machine=tandy`, `cputype=8086`, 640 KB, and a fixed 360-cycle setting. The capture completed the intro and seven full arrangement loops and exited cleanly. This is emulator testing; it has not been tested on a physical Tandy 1000 EX.

## Credits and licenses

- Original NyanCat-XT source and project attribution: [OlivierP-To8/NyanCat-XT](https://github.com/OlivierP-To8/NyanCat-XT), licensed under GPL-3.0-or-later as retained in [`LICENSE`](LICENSE).
- Vendored static-score/PSG subset: [astrobleem/libtandy](https://github.com/astrobleem/libtandy), MIT licensed. The pinned source commit, file manifest, and adapter notes are in [`third_party/libtandy/README.md`](third_party/libtandy/README.md) and [`third_party/libtandy/MANIFEST-SHA256.txt`](third_party/libtandy/MANIFEST-SHA256.txt); see its included `LICENSE`.
- The static note and percussion events are based on Tyler's "Nyan Cat" MIDI listing on VGMusic; the source URL and arrangement notes are preserved in [`SRC/TANDY.C`](SRC/TANDY.C).

