/*
 * This file is part of the Linux desktop port of Kick.
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

namespace desktop
{
    // Mirror of what the app asks the hardware panel to display: pad colours, button LEDs,
    // the selected bank and the touch strip level. Fed by Controls::InputEvents::hardwareOutput
    // from any thread, read on the message thread.
    class HardwareState : private juce::AsyncUpdater, private juce::DeletedAtShutdown
    {
    public:
        static constexpr int numPads    = 16;
        static constexpr int numButtons = contracts::DeviceControlContracts::SIZE;

        struct Pad
        {
            int  colour{ contracts::DeviceControlContracts::PadColourTrans };
            int  offColour{ contracts::DeviceControlContracts::PadColourTrans }; // second colour while blinking
            int  state{ contracts::DeviceControlContracts::StateOff };
            int  velocity{ 127 };
            bool operator!=( const Pad& o ) const { return colour != o.colour || offColour != o.offColour || state != o.state || velocity != o.velocity; }
        };

        static HardwareState& get() { return *getInstance(); }
        JUCE_DECLARE_SINGLETON( HardwareState, false )

        const Pad& getPad( int index ) const { return pads[(size_t)index]; }
        bool       isLit( int buttonId ) const { return juce::isPositiveAndBelow( buttonId, numButtons ) && leds[(size_t)buttonId]; }
        int        getSliderLevel() const { return sliderLevel; }

        // Called on the message thread after a batch of changes.
        std::function<void()> onChange;

    private:
        HardwareState();
        ~HardwareState() override;

        void handleAsyncUpdate() override;
        void apply( const CPktMgr::HWMsg_t& msg );
        void applyPad( const CPktMgr::HWMsg_t& msg );
        void applyLed( int id, bool on );

        juce::CriticalSection          lock;
        std::vector<CPktMgr::HWMsg_t> pending;

        std::array<Pad, numPads>     pads{};
        std::array<bool, numButtons> leds{};
        int                          sliderLevel{ 0 };
        bool                         changed{ false };

        sigslot::scoped_connection connection;
    };
}
