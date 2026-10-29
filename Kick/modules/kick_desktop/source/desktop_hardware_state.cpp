/*
 * This file is part of the Linux desktop port of Kick.
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "input_events/input_events.h"

namespace desktop
{
    using DCC = contracts::DeviceControlContracts;

    JUCE_IMPLEMENT_SINGLETON( HardwareState )

    HardwareState::HardwareState()
    {
        connection = Controls::InputEvents::hardwareOutput.connect( [this]( const CPktMgr::HWMsg_t& msg ) {
            {
                const juce::ScopedLock sl( lock );
                pending.push_back( msg );
            }
            triggerAsyncUpdate();
        } );
    }

    HardwareState::~HardwareState()
    {
        connection.disconnect();
        cancelPendingUpdate();
        clearSingletonInstance();
    }

    void HardwareState::handleAsyncUpdate()
    {
        std::vector<CPktMgr::HWMsg_t> batch;
        {
            const juce::ScopedLock sl( lock );
            batch.swap( pending );
        }

        for( const auto& msg : batch )
            apply( msg );

        if( changed && onChange )
            onChange();
        changed = false;
    }

    void HardwareState::apply( const CPktMgr::HWMsg_t& msg )
    {
        const auto& d = msg.data;
        switch( msg.pktType )
        {
            case CPktMgr::EN_PKT_TYPE_PAD:
                applyPad( msg );
                break;

            case CPktMgr::EN_PKT_TYPE_LED:
                if( msg.pktCmd == DCC::EN_LED_SET_SINGLE_LED && d.size() >= 2 )
                    applyLed( d[0], d[1] != DCC::StateOff );
                break;

            case CPktMgr::EN_PKT_TYPE_BUTTON:
                // setPlayStateMessage() lights a transport button this way.
                if( msg.pktCmd == DCC::EN_BTN_STATUS && d.size() >= 2 )
                    applyLed( d[0], d[1] == DCC::BTN_PRESSED );
                break;

            case CPktMgr::EN_PKT_TYPE_SLIDER:
                if( msg.pktCmd != DCC::EN_SLIDER_TOUCH_INPUT && !d.empty() && sliderLevel != d[0] )
                {
                    sliderLevel = juce::jlimit( 0, 100, d[0] );
                    changed     = true;
                }
                break;

            default:
                break;
        }
    }

    void HardwareState::applyLed( int id, bool on )
    {
        if( !juce::isPositiveAndBelow( id, numButtons ) )
            return;

        // One bank is selected at a time, and play / stop exclude each other.
        if( on && id <= DCC::EN_BANK_H )
            for( int b = DCC::EN_BANK_A; b <= DCC::EN_BANK_H; ++b )
                leds[(size_t)b] = false;
        if( on && ( id == DCC::EN_PLAY || id == DCC::EN_PLAYSTART ) )
            leds[DCC::EN_STOP] = false;
        if( on && id == DCC::EN_STOP )
            leds[DCC::EN_PLAY] = leds[DCC::EN_PLAYSTART] = false;

        leds[(size_t)id] = on;
        changed          = true;
    }

    void HardwareState::applyPad( const CPktMgr::HWMsg_t& msg )
    {
        const auto& d   = msg.data;
        const auto  num = (int)d.size();

        auto set = [this]( int index, Pad p ) {
            if( juce::isPositiveAndBelow( index, numPads ) && pads[(size_t)index] != p )
            {
                pads[(size_t)index] = p;
                changed             = true;
            }
        };

        switch( msg.pktCmd )
        {
            case DCC::EN_PAD_SET_SINGLE_PAD: // pad, state, colour
                if( num >= 3 ) set( d[0], { d[2], DCC::PadColourTrans, d[1], 127 } );
                break;

            case DCC::EN_PAD_SET_ALL_SINGLE_COLOUR: // state, colour
                if( num >= 2 )
                    for( int i = 0; i < numPads; ++i ) set( i, { d[1], DCC::PadColourTrans, d[0], 127 } );
                break;

            case DCC::EN_PAD_SET_ALL_MULTI_COLOUR: // state, colour x16
                for( int i = 0; i < numPads && i + 1 < num; ++i )
                    set( i, { d[(size_t)i + 1], DCC::PadColourTrans, d[0], 127 } );
                break;

            case DCC::EN_PAD_SET_COLOUR_STATE: // colour xN, state xN
                for( int i = 0; i < num / 2; ++i )
                    set( i, { d[(size_t)i], DCC::PadColourTrans, d[(size_t)( num / 2 + i )], 127 } );
                break;

            case DCC::EN_PAD_SET_BLINKING: // pad, state, on colour, off colour
                if( num >= 4 ) set( d[0], { d[2], d[3], d[1], 127 } );
                break;

            case DCC::EN_PAD_SET_STEP_SINGLE_PAD: // pad, state, colour, velocity
                if( num >= 4 ) set( d[0], { d[2], DCC::PadColourTrans, d[1], d[3] } );
                break;

            case DCC::EN_PAD_SET_STEP_ALL_PAD: // state, colour x16, velocity x16
                for( int i = 0; i < numPads && 1 + numPads + i < num; ++i )
                    set( i, { d[(size_t)i + 1], DCC::PadColourTrans, d[0], d[(size_t)( 1 + numPads + i )] } );
                break;

            default:
                break;
        }
    }
}
