/*
 * This file is part of the Linux desktop port of Kick.
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "contracts/dcl/dcl_contracts.h"
#include "contracts/utils/keyboard/keyboard_signals.h"

namespace desktop
{
    using DCC = contracts::DeviceControlContracts;

    static void dispatch( CPktMgr::enumPacketType type, int cmd, std::vector<int> data )
    {
        jassert( juce::MessageManager::existsAndIsCurrentThread() );

        CPktMgr::HWMsg_t msg;
        msg.pktType = type;
        msg.pktCmd  = cmd;
        msg.data    = std::move( data );
        contracts::KeyboardSignals::KeyboardEventTriggered( msg );
    }

    void sendButton( int buttonId, bool pressed )
    {
        dispatch( CPktMgr::EN_PKT_TYPE_BUTTON, DCC::EN_BTN_STATUS, { buttonId, pressed ? DCC::BTN_PRESSED : DCC::BTN_RELEASED } );
    }

    void sendPad( int pad, bool pressed, int velocity )
    {
        velocity = juce::jlimit( 1, 127, velocity );
        dispatch( CPktMgr::EN_PKT_TYPE_PAD, DCC::EN_PAD_STATE, { pad, pressed ? DCC::BTN_PRESSED : DCC::BTN_RELEASED, velocity, 0 } );
    }

    void sendKnob( int knob, int steps )
    {
        if( steps == 0 )
            return;

        // The encoder reports a direction and a step count that fits a signed byte.
        auto direction = steps > 0 ? DCC::EN_KNOB_CLOCKWISE : DCC::EN_KNOB_ANTICLOCKWISE;
        auto count     = juce::jmin( std::abs( steps ), 127 );
        dispatch( CPktMgr::EN_PKT_TYPE_KNOBS, DCC::EN_KNOB_ROTARY, { knob, direction, count, 0 } );
    }

    void sendSlider( int percent )
    {
        dispatch( CPktMgr::EN_PKT_TYPE_SLIDER, DCC::EN_SLIDER_TOUCH_INPUT, { juce::jlimit( 0, 100, percent ) } );
    }
}
