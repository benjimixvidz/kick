# CLAUDE.md

## Project

Personal, unofficial project: a **Linux desktop build** of the open-source application
software of the Behringer KICK (BE 0700-AAM), usable with **keyboard and mouse** in a regular
desktop environment (no touchscreen, pads, knobs or jog wheel required).

- Upstream source: https://github.com/MusicTribeOpenSource/0700-AAM, published by Music Tribe.
  C++17 / CMake / JUCE 7.0.12. Vendored here: `Kick/` (application), `AudIO/` (audio engine),
  `ComProtocol/` (hardware packet protocol). Provenance and layers: `UPSTREAM.md`.
- The release lacks `AudIO_constants`, `AudIO_juce` and `juce_serialport`; community stand-ins
  (Eight4aWish, `macos-build` branch) fill them in. Values marked "guess" may be wrong.
- Work is done **from source only**. Upstream code is open, so no binary analysis is needed.

## Layout of the port

- `KICK_DESKTOP` (CMake option, on for native Linux builds) switches the app to desktop mode.
  Changes to upstream files are small and guarded by `#if defined( KICK_DESKTOP )`.
- `Kick/modules/kick_desktop/`: everything desktop-only (window and zoom, keyboard map by
  physical key position, on-screen control panel, mirror of the LED/pad messages, audio
  settings dialog).
- Hardware input and output: the control surface speaks `CPktMgr::HWMsg_t`. Desktop input is
  injected through `contracts::KeyboardSignals::KeyboardEventTriggered`; messages for the
  hardware come out of `Controls::InputEvents::hardwareOutput`.
- Device shell commands (network, clock, power, firmware) go through `HwPref::executeCommand`
  and `executeSetCommand`, which do nothing on desktop. Keep it that way for any new one.
- `build.sh` builds, installs (`--install`) or runs (`--run`); `tests/audio-device-fifo` checks
  the audio block adapter.

This repository is **not a fork yet**. It holds the desktop port while it is tested; the plan
is to move the work to a proper fork and propose changes upstream once it works.

## Public repository: what never goes in

This repository is public. Never commit:

- Secrets of any kind: tokens, API keys, passwords, SSH keys, `.env` files, CI credentials.
- Private URLs, internal hostnames, IP addresses, or personal data (emails, names, paths from
  someone's machine beyond generic ones).
- Material upstream removed because it could not be redistributed: factory audio samples,
  the Myriad Pro font, the prebuilt aarch64 SDK, `juce_serialport`. Use free replacements.
- Build output, downloaded dependencies (`build*/`, CPM cache, JUCE checkout).

When in doubt, leave it out and ask.

## Licensing and naming

- Upstream Music Tribe code (Kick, AudIO, ComProtocol) is **AGPL-3.0-or-later**; anything
  derived from it here is AGPL-3.0-or-later too. Keep upstream copyright and license notices.
- Third-party components keep their own licenses (see upstream READMEs).
- "Behringer", "KICK" and their logos are Music Tribe trademarks and are **not** covered by the
  software license. Do not use them as this project's name or branding; mention them only to
  say what the software is compatible with.

## Next steps

- Real-desktop testing: sound by ear, PipeWire/JACK latency, HiDPI, Wayland via XWayland.
- User-editable key map file; free replacement sounds (CC0) for demos and metronome.
- Packaging (AppImage or Flatpak).
- Propose the guarded fixes upstream (shift state, keyboard on Linux, audio block size).

## Conventions

- Keep changes to upstream code minimal and guarded (`#if defined(KICK_DESKTOP)`), so they can
  be proposed upstream as clean patches.
- Match upstream style in upstream files; write new desktop-specific code in its own files.
- Out-of-tree builds only: `./build.sh` (Release, LTO). For quick iterations configure a
  `RelWithDebInfo` build the same way as `build.sh` does; it skips LTO.
- The engine runs fixed 480-sample blocks at 48 kHz (`AUDIO_BLOCK_SIZE`, `AUDIO_SAMPLE_RATE`).

- Linux build dependencies (Debian/Ubuntu): `cmake ninja-build g++ libasound2-dev
  libjack-jackd2-dev libx11-dev libxext-dev libxrandr-dev libxinerama-dev libxcursor-dev
  libxcomposite-dev libxi-dev libxrender-dev libfreetype-dev libfontconfig1-dev
  libcurl4-openssl-dev libgl1-mesa-dev libglu1-mesa-dev`.
- Write comments, commits and docs in English; discussion with the maintainer may be in French.
