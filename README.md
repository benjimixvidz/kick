# Kick desktop: unofficial Linux port

A desktop Linux build of the open-source application software of the Behringer KICK
(BE 0700-AAM), played with a regular keyboard and mouse in a normal window.

Unofficial and not affiliated with Music Tribe. It is built from the source Music Tribe
published at [MusicTribeOpenSource/0700-AAM](https://github.com/MusicTribeOpenSource/0700-AAM),
plus community stand-ins for the parts that release left out. See [UPSTREAM.md](UPSTREAM.md).

## Status

- Builds and runs on x86_64 Linux. So far tested on Ubuntu 24.04 (GCC 13) in a headless
  session, without a real screen or sound card: try it on your desktop and report back.
- Window with the 1024x600 app screen and an on-screen control panel (pads, Q-Link knobs,
  jog wheel, touch strip, banks, transport) that lights up like the hardware.
- Full keyboard control, laid out by physical key position, so it works the same on
  QWERTY, AZERTY, QWERTZ and other layouts.
- Audio through JUCE: ALSA or JACK, and PipeWire through either.
- The audio engine glue is a community reconstruction (see Limitations).

## Build and run

Install the dependencies (Debian/Ubuntu names; other distributions have equivalents):

```bash
sudo apt install cmake ninja-build g++ pkg-config git libasound2-dev libjack-jackd2-dev libx11-dev libxext-dev libxrandr-dev libxinerama-dev libxcursor-dev libxcomposite-dev libxi-dev libxrender-dev libfreetype-dev libfontconfig1-dev libcurl4-openssl-dev libgl1-mesa-dev libglu1-mesa-dev
```

Build and start it:

```bash
./build.sh --run
```

Install it for your user (binary in `~/.local/bin`, a launcher in the applications menu):

```bash
./build.sh --install
```

The first build downloads JUCE 7.0.12 into `.cache/` and takes several minutes.

## Controls

The touchscreen is the mouse: click, drag and scroll on the app screen.

### Control panel (mouse)

| Control | Mouse |
|---|---|
| Pads | Click; a click near the top of a pad hits harder (higher velocity) |
| Q-Link knobs Q1 to Q4 | Drag up or down, or scroll |
| Jog wheel | Drag around it or scroll; click the centre to push |
| Touch strip | Click or drag along it |
| Banks, buttons | Click |
| MODES | Menu of every hardware button, with its key on your keyboard |

### Keyboard

Keys are listed as they sit on a US keyboard. On another layout use the key in the same
place: every pad shows its key, and the MODES menu lists them all.

| Keys | Hardware |
|---|---|
| `1 2 3 4` / `Q W E R` / `A S D F` / `Z X C V` | Pads 13-16 / 9-12 / 5-8 / 1-4 |
| `F1` to `F4`, `F6` to `F8` | Banks A to D, F to H |
| `F9` `F10` `F11` `Space` `F12` | Record, Overdub, Stop, Play, Play Start |
| `Up` `Down` / `Enter` | Turn / push the jog wheel |
| `Left` `Right` | Minus, Plus |
| `Shift` | Shift |
| `T` `Y` `U` `I` `O` | Main, Browse, Track Mix, Track Mute, Next Seq |
| `5` `6` `7` `8` `9` | Grid, Save, Pad Mix, Pad Mute, XY FX |
| `G` `H` `J` `K` `L` | Step Seq, Time Correct, Sampler, Sample Edit, Program Edit |
| `B` `N` `M` `,` `.` | Automation, On/Off, Looper, Link Edit, MIDI Control |
| `0` `-` `=` `P` `[` `]` | Full Level, Note Repeat, 16 Levels, Half Level, Latch, Notes |
| `Esc` `Insert` `Delete` `Tab` | Menu, Preferences, Erase, Link |
| `Page Up` `Page Down` | Back / Undo, Copy |

### Window

| Keys | Action |
|---|---|
| `Ctrl +` / `Ctrl -` / `Ctrl 0` | Zoom in / out / default zoom |
| `Ctrl F` or `Alt Enter` | Full screen |
| `Ctrl P` | Show or hide the control panel |
| `Ctrl ,` | Audio settings |
| `Ctrl Q` | Quit |

Zoom, full screen and the panel are remembered. `Esc` is the hardware Menu button, so it
does not leave full screen.

## Audio

The app opens your default audio device. Pick another in the audio settings (`Ctrl ,`); the
choice is remembered. MIDI devices are set up inside the app, as on the hardware.

The engine runs at 48 kHz in blocks of 480 samples (10 ms). A device buffer of 480 or 960
samples adds no latency; any other size works too, with up to 10 ms more.

## Your files

| What | Where |
|---|---|
| Projects | `~/.local/share/kick-desktop/projects` |
| Demos (your copy, saved like a project) | `~/.local/share/kick-desktop/demos` |
| Your samples | `~/.local/share/kick-desktop/sounds` |
| Settings | `~/.config/kick-desktop` |
| Logs | `~/.local/state/kick-desktop/logs` |

The XDG variables (`XDG_DATA_HOME` and so on) are honoured. `KICK_DATA_DIR` can point to
another folder holding `assets/`, `demos/` and `templates/`. The demos are copied to your
data folder the first time they are missing there; delete one from it to get the original back.

## Limitations

- Factory sounds and the metronome samples are not part of the open-source release, so the
  demos load without their sounds. Bring your own samples.
- Music Tribe did not publish two audio engine modules (`AudIO_constants`, `AudIO_juce`).
  This port uses community reconstructions (MusicTribeOpenSource/0700-AAM#9); some values
  are educated guesses, so the sound may differ from the hardware in places.
- No AI camera, CV/Gate, firmware update, network or storage settings: those are hardware
  features. Device commands are disabled in this build.
- Audio timing is covered by an automated test (`tests/audio-device-fifo`), but the sound
  itself has not been listened to on this port yet.
- X11 only (JUCE 7). On Wayland it runs through XWayland.

## Licence and trademarks

The software is licensed under the GNU Affero General Public License v3.0 or later, as is
the Music Tribe code it is built from (see [LICENSE](LICENSE)). Third-party components keep
their own licences.

Behringer, KICK and their logos are trademarks of Music Tribe. They are used here only to
say what this software is compatible with; this project is not endorsed by Music Tribe.
