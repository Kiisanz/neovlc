# NeoVLC

An experimental desktop media player built with **C++17**, **Qt Quick/QML**, and **libVLC**.

NeoVLC is a personal research and hobby project exploring modern media player interfaces, declarative UI development, and native playback integration. It is being developed in the open, with the aim of becoming an approachable open-source project for learning, experimentation, and collaboration.

[Repository](https://github.com/Kiisanz/neovlc) · [Architecture](docs/architecture.md) · [Roadmap](#roadmap)

## Motivation and Design Reference

The project takes design inspiration from [“I Redesigned VLC After 25 Years”](https://www.youtube.com/watch?v=ERFeOgqk324) by **Juxtopposed**. The video serves as a reference for exploring how a contemporary interface can support familiar media playback workflows.

The research focuses on three areas:

- Translating visual concepts into reusable QML components.
- Integrating libVLC with Qt Quick while keeping playback logic separate from presentation.
- Developing a modular C++ application with clear ownership and predictable resource lifetimes.

The current interface is a placeholder. Implementation of the design reference is planned as part of the UI milestone.

## Project Status

**Early development · Pre-MVP**

The project currently provides an application scaffold and initial libVLC integration, including local media opening, transport controls, playback-state reporting, and in-memory video output rendered through Qt Quick.

| Area | Current state |
| --- | --- |
| Architecture | Separate application, VLC backend, and QML UI modules |
| Build configuration | CMake definitions for Qt 6 and libVLC via pkg-config |
| libVLC dependency | Version 3.0.24, development headers, and runtime plugins detected in the setup environment |
| Backend foundation | Instance and media player ownership, lifecycle safeguards, version information, and error reporting |
| Media opening | Local video files validated and loaded through the QML file picker |
| Transport controls | Play, pause, stop, seek, and volume in the backend; Play/Pause/Stop buttons, seek bar, and volume slider in QML |
| Playback state | State, position, duration, and playback errors exposed to QML with time labels |
| QML integration | Backend, player, and video sink exposed through `vlcBackend`, `vlcPlayer`, and `vlcVideo`; a `VideoItem` draws frames in the scene graph |
| Interface | Basic window with file-open, transport, seek, and volume controls; design implementation pending |
| Video rendering | libVLC video callbacks deliver `RV32` frames to a Qt Quick scene-graph item with aspect-ratio letterboxing |
| Verification | CMake configuration and compilation verified in the setup environment; lifecycle, open-file, playback-control, playback-state, and video-output smoke tests passed; QML imports and startup verified offscreen; scene-graph frame presentation and letterboxing verified offscreen; interactive controls not verified |

Development is exploratory and follows the roadmap below rather than a fixed release schedule. Linux is the initial development environment; other platforms have not been validated.

## Roadmap

Checked items indicate source code or documentation already prepared. Build and runtime verification are tracked separately.

### 1. Project Foundation

- [x] Establish the C++17 and Qt Quick/QML application scaffold.
- [x] Separate application composition, VLC integration, and UI modules.
- [x] Define libVLC discovery and linking through CMake and pkg-config.
- [x] Implement libVLC instance ownership and expose backend status to QML.
- [x] Document module boundaries and the design reference.

### 2. Playback Backend

- [x] Implement media player ownership and lifecycle management.
- [x] Open local video files.
- [x] Add play, pause, stop, seek, and volume controls.
- [x] Expose playback state, position, duration, and errors to QML.
- [x] Integrate libVLC video output with Qt Quick.

**Milestone:** Local video can be rendered and controlled through the playback backend.

### 3. Player Interface

- [ ] Translate the design reference into QML layouts and reusable components.
- [ ] Establish consistent colors, typography, spacing, and iconography.
- [ ] Build the video surface, file selection flow, and transport controls.
- [ ] Add a seek bar, time indicators, and volume controls.
- [ ] Support fullscreen, essential keyboard shortcuts, and empty and error states.

**Milestone:** A usable MVP for opening and watching local video files.

### 4. Extended Media Features

- [ ] Playlists and playback queues.
- [ ] Subtitle and audio track selection.
- [ ] Playback speed controls.
- [ ] Recent media and persistent user preferences.
- [ ] Network media sources.

### 5. Validation and Distribution

- [ ] Verify CMake configuration and compilation.
- [ ] Test playback, seeking, audio output, UI synchronization, and application shutdown.
- [ ] Check required media formats and VLC runtime plugins.
- [ ] Evaluate additional target platforms.
- [ ] Add focused automated tests for backend behavior.
- [ ] Prepare packaging, runtime dependency handling, and setup instructions.

Verification will accompany feature development. The final milestone consolidates release readiness; distribution targets will be determined after the MVP is validated.

## Requirements

- A C++17 compiler
- CMake 3.16 or later
- Qt 6.2 or later with Core, Gui, Quick, and Quick Controls 2
- pkg-config
- libVLC 3.x, version 3.0 or later, including development headers and runtime plugins

### libVLC Discovery

CMake uses the imported target `PkgConfig::LIBVLC` to obtain the required include paths and linker flags. libVLC is a required dependency.

Check the local installation without configuring or building the application:

```bash
pkg-config --modversion libvlc
pkg-config --cflags --libs libvlc
```

For a custom installation prefix, add the directory containing `libvlc.pc` to `PKG_CONFIG_PATH`. The VLC shared libraries and runtime plugins must also be available when the application runs. Runtime bundling is not implemented yet.

The setup environment is CachyOS Linux, where libVLC 3.0.24 was detected. Any existing `build/` directory contains artifacts from the earlier scaffold and does not validate the current implementation.

## Project Structure

```text
neovlc/
├── CMakeLists.txt              # Project settings and Qt dependencies
├── src/
│   ├── CMakeLists.txt          # Application target composition
│   ├── app/
│   │   ├── Application.cmake   # Application sources and dependencies
│   │   ├── main.cpp            # Entry point and application metadata
│   │   ├── Application.h
│   │   └── Application.cpp     # Backend and QML lifecycle composition
│   ├── backend/vlc/
│   │   ├── CMakeLists.txt      # NeoVLC::Vlc library and libVLC discovery
│   │   ├── VlcContext.h        # libVLC instance ownership and player registry
│   │   ├── VlcContext.cpp      # libVLC instance implementation
│   │   ├── VlcMediaPlayer.h    # Media player ownership interface
│   │   ├── VlcMediaPlayer.cpp  # Media player lifecycle implementation
│   │   ├── VlcVideoOutput.h    # In-memory video frame sink
│   │   └── VlcVideoOutput.cpp  # libVLC video callbacks and frame delivery
│   └── ui/
│       ├── Ui.cmake            # QML module, video item, and resource registration
│       ├── VideoItem.h         # QML item that draws video frames
│       ├── VideoItem.cpp       # Scene-graph texture upload and letterboxing
│       └── Main.qml            # Application window
└── docs/
    └── architecture.md        # Module boundaries and development guidance
```

The application layer composes the backend and UI. The VLC backend is a separate static library, `NeoVLC::Vlc`, with no dependency on QML or Qt Quick. The UI is packaged in the executable as the `NeoVLC` QML module, with `qrc:/NeoVLC/Main.qml` as its entry point.

QML can read `vlcBackend.available`, `vlcBackend.version`, `vlcBackend.errorString`, `vlcPlayer.available`, `vlcPlayer.errorString`, `vlcPlayer.mediaPath`, `vlcPlayer.volume`, `vlcPlayer.playbackState`, `vlcPlayer.playbackStateName`, `vlcPlayer.position`, and `vlcPlayer.duration`. The interface provides an Open File button and `FileDialog` that call `vlcPlayer.openFile(selectedFile)`, Play/Pause/Stop buttons, a seek bar with current/total time labels, a playback-state label, and a 0–200% volume slider. The backend also exposes `vlcPlayer.play()`, `vlcPlayer.pause()`, `vlcPlayer.stop()`, `vlcPlayer.seek(milliseconds)`, and `vlcPlayer.setVolume(percent)`. Playback state and position/duration are updated from libVLC events on the Qt thread. Transport calls without media are rejected through `errorString`, and playback failures set the `error` state with a message. The header shows the loaded `mediaPath`, and the window reports `errorString`. Missing files and non-local URLs are rejected through `errorString`. A `true` return from `openFile()` means the media descriptor was assigned; playability is validated later by libVLC during parsing and playback. The application creates the player after `VlcContext` and before the QML engine, so destruction runs engine first, then player, then the libVLC instance. `VlcContext` tracks live players, asserts the destruction order in debug builds, and warns in release builds if a player outlives the instance, detaching it so teardown cannot touch the destroyed context. Future C++ playback components can access the shared libVLC instance through `VlcContext::instance()` and must also be destroyed before its owning context.

See [Architecture](docs/architecture.md) for module responsibilities and ownership rules.

## Contributing

Discussion, technical feedback, bug reports, and focused contributions are welcome. Useful areas of exploration include refining Qt Quick video rendering performance, libVLC lifecycle management, accessible player controls, and testing across platforms.

For substantial changes, open an issue to discuss the approach first. Keep contributions scoped to a clear problem, respect the existing module boundaries, and describe any verification performed.

## License

Project licensing is pending; the repository does not yet include a `LICENSE` file.

## References

- [“I Redesigned VLC After 25 Years” — Juxtopposed](https://www.youtube.com/watch?v=ERFeOgqk324)
- [VideoLAN libVLC 3.0 API](https://videolan.videolan.me/vlc-3.0/group__libvlc__core.html)
