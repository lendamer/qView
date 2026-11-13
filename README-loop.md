# qView Loop

Local fork of https://github.com/jurplel/qView with original-file MP4 playback in the existing image viewer. MP4s autoplay and repeat in mixed image/video folders. Audio starts muted.

- Left/Right: previous/next file.
- P: pause/resume GIF or MP4.
- M: mute/unmute video.
- Use the existing zoom, original-size, rotation and fullscreen actions.
- GIF frame stepping and saving remain GIF-only. Copying a video copies its file URL.

The fork uses a separate app/settings identity. It does not replace installed qView or change default file associations.

## Build on this Mac

Install Xcode command-line tools, CMake, and Homebrew dependencies:

```sh
brew install qtbase qtmultimedia qtsvg qtimageformats
bash scripts/build-macos-loop.sh
```

The script compiles, runs native GUI tests, bundles Qt and the multimedia backend, and ad-hoc signs `../qView Loop.app`. Choose a different destination as its first argument when rebuilding. Tests require a graphical login session on macOS; the offscreen backend is incompatible with upstream Cocoa window helpers.

The local build uses Homebrew Qt 6.12 and targets this Apple Silicon Mac. For other Macs, rebuild with dependencies suitable for their macOS version. Qt 6.5 or newer is required.

## Playback limits

The original MP4 is decoded at source resolution without transcoding or GIF conversion. Codec, color, HDR and loop-boundary behavior depend on Qt and its deployed backend. Full HDR/10-bit fidelity and frame-perfect gapless loops are not guaranteed. The scene renderer may use CPU rendering; very large/high-frame-rate videos can be demanding.

The app is locally ad-hoc signed, not notarized for public distribution. Upstream licensing remains in LICENSE.
