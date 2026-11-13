# MP4 looping fork

The user requested a local Mac qView fork with MP4 playback at source resolution, looping automatically like a GIF. They confirmed videos belong in mixed image/video folder navigation with existing zoom and fullscreen, then explicitly said to proceed with Qt Multimedia and delegate implementation choices.

Use QMediaPlayer with infinite loops and QGraphicsVideoItem in the existing graphics scene. Retain image/GIF rendering. Do not transcode, pre-resize source files, or cache entire videos. The graphics scene handles video fit, zoom, pan, rotation, mirror and fullscreen. Qt decoding/rendering determines codec, color and HDR support; do not promise HDR fidelity or frame-perfect seamless looping without evidence.

Audio is included, muted by default to match GIF behavior, with an M mute/unmute action. Pause/resume and speed work for both GIF and MP4. Frame stepping/saving remain GIF-specific. MP4 appears in open filters and folder sorting and gets a Finder document association. Release playback on replacement, close and file mutation; asynchronous callbacks must not revive old media. Corrupt media must remain navigable. Image preloading skips MP4.

Build with Qt6 Multimedia/MultimediaWidgets. Deliver a separately named qView Loop.app on the Desktop, a distinct bundle/settings identity, local source and reproducible build instructions. Keep the existing installed qView untouched. No remote publication is requested.

Alternatives considered: libmpv offers richer controls but adds a dependency; converting frames into pixmaps reuses rendering but adds frame conversion and scaling work. Qt's video scene item is the selected implementation choice, not a guarantee of hardware rendering.

Verify short H.264 MP4 loops, source dimensions, pause/rate/mute, mixed folder navigation, rapid replacement, corrupt media, rotation/zoom preservation, GIF/image regressions, existing tests, and deployed app startup/playback. Add small generated media fixtures and real integration tests.
