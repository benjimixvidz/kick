/*
 * This file is part of the Linux desktop port of Kick.
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

namespace desktop
{
    // Inject control-surface input exactly as the hardware panel reports it, so the app
    // cannot tell the on-screen panel and the keyboard from the real device. Message thread.
    void sendButton( int buttonId, bool pressed );
    void sendPad( int pad, bool pressed, int velocity );
    void sendKnob( int knob, int steps ); // positive steps turn clockwise
    void sendSlider( int percent );       // touch strip, 0..100
}
