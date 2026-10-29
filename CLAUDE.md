# CLAUDE.md

## Project

Personal, unofficial project: a **Linux desktop build** of the open-source application
software of the Behringer KICK (BE 0700-AAM), usable with **keyboard and mouse** in a regular
desktop environment (no touchscreen, pads, knobs or jog wheel required).

- Upstream source: https://github.com/MusicTribeOpenSource/0700-AAM, published by Music Tribe.
- Upstream is C++17 / CMake / JUCE 7.0.12. Main folders: `Kick/` (application), `AudIO/` (audio
  engine, JUCE module), `ComProtocol/` (hardware packet protocol), `Linux/` (Buildroot image,
  not needed for desktop).
- Upstream already ships a desktop simulator (`Kick/apps/kickem`, CMake option
  `KICK_SIMULATOR`, define `SIM_BUILD`). This project builds on it instead of starting over.
- Work is done **from source only**. Upstream code is open, so no binary analysis is needed.

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

## Goals for the desktop port

1. Build and run the `kickem` simulator on x86_64 Linux (X11 first, Wayland via XWayland).
2. Replace the hardware input path (serial link in `Kick/modules/input_events`) with a desktop
   path guarded by a `KICK_DESKTOP` define. Never break the device build.
3. Full keyboard and mouse control: pads, banks, transport, knobs, jog wheel, touch strip,
   mapped to keys and mouse, with a configurable keymap.
4. Audio through JUCE (ALSA, JACK; PipeWire through its JACK/ALSA layers). MIDI in/out through
   JUCE's ALSA MIDI.
5. Resizable window, sensible defaults, packaging (AppImage or Flatpak) later.

## Conventions

- Keep changes to upstream code minimal and guarded (`#if defined(KICK_DESKTOP)`), so they can
  be proposed upstream as clean patches.
- Match upstream style in upstream files; write new desktop-specific code in its own files.
- Out-of-tree builds only. Configure example (paths are placeholders):

  ```bash
  cmake -G Ninja <upstream>/Kick -DKICK_SIMULATOR=ON -DTIMESTAMP=OFF \
    -DCMAKE_CXX_FLAGS=-DKICK_DESKTOP=1 \
    -DCPM_AudIO_SOURCE=<audio-dir> -DCPM_COM_PROTOCOL_SOURCE=<upstream>/ComProtocol
  ```

- Linux build dependencies (Debian/Ubuntu): `cmake ninja-build g++ libasound2-dev
  libjack-jackd2-dev libx11-dev libxext-dev libxrandr-dev libxinerama-dev libxcursor-dev
  libxcomposite-dev libxi-dev libxrender-dev libfreetype-dev libfontconfig1-dev
  libcurl4-openssl-dev libgl1-mesa-dev libglu1-mesa-dev`.
- Write comments, commits and docs in English; discussion with the maintainer may be in French.
