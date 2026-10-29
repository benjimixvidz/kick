/*
 * This file is part of the Linux desktop port of Kick.
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

namespace desktop
{
    // On-screen stand-in for the hardware controls next to the touchscreen: Q-Link knobs,
    // jog wheel, touch strip, banks, pads and transport. It sends exactly what the hardware
    // panel sends and lights up from what the app sends back (HardwareState).
    class ControlPanel : public juce::Component, private juce::Timer
    {
    public:
        static constexpr int width  = 440;
        static constexpr int height = 600;

        ControlPanel();
        ~ControlPanel() override;

        // Refreshes the key hints after the key map changed.
        void setKeyMap( const KeyMap& map );

        // Items for the window's shortcuts section of the modes menu.
        std::function<void( juce::PopupMenu& )> addWindowItems;

        void paint( juce::Graphics& g ) override;
        void resized() override;

        class PanelButton;
        class PadButton;
        class Encoder;
        class TouchStrip;

    private:
        void timerCallback() override;
        void showModesMenu();
        void refresh();

        std::vector<std::unique_ptr<Encoder>>     knobs;
        std::unique_ptr<Encoder>                  jog;
        std::unique_ptr<TouchStrip>               strip;
        std::vector<std::unique_ptr<PanelButton>> banks, sideButtons, transport;
        std::vector<std::unique_ptr<PadButton>>   pads;
        std::unique_ptr<PanelButton>              modes;
        KeyMap                                    keyMap;
        bool                                      blinkPhase{ false };
        sigslot::scoped_connection                inputConnection;
    };
}
