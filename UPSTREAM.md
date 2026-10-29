# Upstream provenance

This repository vendors source code from the following public repositories. Their history
is kept in separate commits so each layer can be updated or dropped on its own.

| Layer | Source | Commit | License |
|---|---|---|---|
| Official release | https://github.com/MusicTribeOpenSource/0700-AAM (`main`) | `27194180ce5fbab3776ebce53e2d4b016597364e` | AGPL-3.0-or-later (Kick, AudIO, ComProtocol) |
| Community stand-ins | https://github.com/Eight4aWish/0700-AAM (`macos-build`) | `96e983b08a3c9c2b7e2bca60e9dd01c4dd81e874` | AGPL-3.0-or-later |
| Linux desktop port | this repository | — | AGPL-3.0-or-later |

## What was imported

`Kick/`, `AudIO/`, `ComProtocol/` and the root `LICENSE`.

Left out on purpose, because they only serve the hardware device:

- `Linux/` (Buildroot image for the device)
- `Kick/CI/` (the vendor's build pipelines)
- `Kick/fileTransfer.sh` (copies builds to a device on the local network)
- `docs/` (README images)

## Community stand-ins

The official release does not include the `AudIO_constants` and `AudIO_juce` modules, nor
`juce_serialport` (see upstream issue MusicTribeOpenSource/0700-AAM#9). The `macos-build`
branch by Eight4aWish provides stand-ins written from how the app uses them. Some constants
are educated guesses, so audio behaviour may differ slightly from the device. If Music Tribe
publishes the original modules, the stand-in commit should be replaced by them.

## Updating

To pull new upstream changes, diff the upstream tree between the recorded commit and the new
one, apply that diff here, and update the table above.
