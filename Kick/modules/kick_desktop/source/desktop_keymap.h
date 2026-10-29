/*
 * This file is part of the Linux desktop port of Kick.
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

namespace desktop
{
    using KeyMap = std::map<int, contracts::keyPressedDetector::cmdInfo>;

    // The panel key map, laid out by physical key position so the pad grid and the mode keys
    // sit in the same place on QWERTY, AZERTY, QWERTZ or any other layout. Printable keys are
    // resolved through the current X11 keyboard layout; the rest are layout independent.
    KeyMap buildKeyMap();

    // A short label for the key that triggers a panel control, e.g. "A" or "F1". Empty if the
    // control has no key.
    juce::String keyLabelForButton( const KeyMap& map, int buttonId );
    juce::String keyLabelForPad( const KeyMap& map, int pad );
}
