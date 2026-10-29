/*
 * This file is part of the Linux desktop port of Kick.
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

namespace desktop
{
    // Makes the audio engine start on the device chosen last time (ALSA, JACK or PipeWire
    // through either). Call before the engine starts its device.
    void restoreAudioSettings();

    // Audio device dialog; the choice is saved when it closes. MIDI devices are handled by the
    // app's own MIDI settings.
    void showAudioSettings();
}
