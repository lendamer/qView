# qView Loop verification

Base upstream commit: b8b993a. Local branch: feature/mp4-loop.

Built on this Apple Silicon Mac with Homebrew Qt 6.12. The final native CTest run passed all three suites (ActionManagerTests, videoplayback, mediaintegration), zero failures, 9.45 seconds. The release packaging workflow repeats that suite.

Real-media tests cover source dimensions, repeat playback, pause/speed/mute, zoom persistence across loops, portrait and metadata-oriented sources, decoded-frame rotation without double rotation, user rotation, replacement/stale callbacks, corrupt MP4 recovery, video clipboard URLs, GIF/still behavior, mixed sorting/navigation, menu enablement and failed rename recovery. Tiny fixtures were generated locally with ffmpeg.

Manual inspection confirmed the 3840 × 2160 30 fps original clip rendered in the development and bundled app; File Info reported 3840 × 2160. Fullscreen playback was also inspected. No transcoding is performed. HDR fidelity and frame-perfect gapless transitions are not guaranteed by this implementation.

Read-only subagent reviews covered playback, integration, packaging and the complete change. Follow-up fixes addressed rotated-frame layout, a pre-show null-window ICC lookup and the rename dialog's nested-event-loop lifetime. Existing Qt deprecation/link deployment warnings and an upstream post-read PNG metadata warning remain; the tests pass.

The app uses a distinct settings/bundle identity. It is kept local on the feature branch; nothing is pushed or published. The original installed qView is untouched. The dedicated clone provides isolation; no extra worktree was created. The user's instruction to proceed with Qt Multimedia waived further design questions.

Desktop File Provider restores Finder metadata on copied framework bundles and prevents strict signature verification there. The packaging script stages outside that domain, installs the locally signed bundle in ~/Applications, and places a Desktop symlink. Only FinderInfo/ResourceFork packaging attributes are cleaned; security attributes are preserved.
