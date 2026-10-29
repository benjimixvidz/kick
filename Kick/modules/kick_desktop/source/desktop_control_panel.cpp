/*
 * This file is part of the Linux desktop port of Kick.
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

namespace desktop
{
    namespace
    {
        using DCC = contracts::DeviceControlContracts;

        namespace colours
        {
            const juce::Colour background{ 0xff121212 };
            const juce::Colour section{ 0xff1a1a1a };
            const juce::Colour button{ 0xff2a2a2a };
            const juce::Colour buttonDown{ 0xff444444 };
            const juce::Colour outline{ 0xff3a3a3a };
            const juce::Colour text{ 0xffd8d8d8 };
            const juce::Colour hint{ 0xff808080 };
            const juce::Colour lit{ 0xffffb000 };
            const juce::Colour padBase{ 0xff1e1e1e };
        }

        juce::Font panelFont( float height, bool bold = false ) { return juce::Font( height, bold ? juce::Font::bold : juce::Font::plain ); }
    }

    //==============================================================================
    class ControlPanel::PanelButton : public juce::Component
    {
    public:
        PanelButton( juce::String labelText, int id )
            : label( std::move( labelText ) ), buttonId( id ) {}

        // When set, a click runs this instead of sending the button to the app.
        std::function<void()> onClick;
        juce::String          hint;

        void paint( juce::Graphics& g ) override
        {
            auto bounds = getLocalBounds().toFloat().reduced( 1.0f );
            auto lit    = HardwareState::get().isLit( buttonId );

            g.setColour( isDown ? colours::buttonDown : ( lit ? colours::lit : colours::button ) );
            g.fillRoundedRectangle( bounds, 5.0f );
            g.setColour( lit ? colours::lit.brighter( 0.3f ) : colours::outline );
            g.drawRoundedRectangle( bounds, 5.0f, 1.0f );

            auto textArea = getLocalBounds().reduced( 3, 2 );
            if( hint.isNotEmpty() )
            {
                g.setColour( lit ? juce::Colours::black.withAlpha( 0.6f ) : colours::hint );
                g.setFont( panelFont( 9.0f ) );
                g.drawText( hint, textArea.removeFromTop( 10 ), juce::Justification::centredRight, false );
            }
            g.setColour( lit ? juce::Colours::black : colours::text );
            g.setFont( panelFont( 11.0f, true ) );
            g.drawFittedText( label, textArea, juce::Justification::centred, 2, 0.8f );
        }

        void setPressed( bool pressed )
        {
            if( pressed != isDown )
            {
                isDown = pressed;
                repaint();
            }
        }

        void mouseDown( const juce::MouseEvent& ) override
        {
            if( onClick )
                onClick();
            else
                sendButton( buttonId, true );
        }

        void mouseUp( const juce::MouseEvent& ) override
        {
            if( !onClick )
                sendButton( buttonId, false );
        }

        const juce::String label;
        const int          buttonId;

    private:
        bool isDown{ false };
    };

    //==============================================================================
    class ControlPanel::PadButton : public juce::Component
    {
    public:
        PadButton( int padIndex, const bool& blinkPhaseRef )
            : index( padIndex ), blinkPhase( blinkPhaseRef ) {}

        juce::String hint;

        bool isBlinking() const
        {
            auto state = HardwareState::get().getPad( index ).state;
            return state == DCC::StateOnBlink || state == DCC::StateOffBlink;
        }

        void paint( juce::Graphics& g ) override
        {
            const auto& pad    = HardwareState::get().getPad( index );
            auto        bounds = getLocalBounds().toFloat().reduced( 1.5f );

            g.setColour( colours::padBase );
            g.fillRoundedRectangle( bounds, 6.0f );

            // How the hardware would light this pad.
            int   colourId = pad.colour;
            float alpha    = 0.10f;
            switch( pad.state )
            {
                case DCC::StateOn:
                case DCC::StateSampleAssigned: alpha = 0.35f + 0.65f * (float)pad.velocity / 127.0f; break;
                case DCC::StateSampleNotAssigned: alpha = 0.18f; break;
                case DCC::StateOnBlink:
                case DCC::StateOffBlink:
                {
                    bool on = ( pad.state == DCC::StateOnBlink ) == blinkPhase;
                    if( !on ) colourId = pad.offColour;
                    alpha = 0.9f;
                    break;
                }
                default: break;
            }

            if( colourId != DCC::PadColourTrans )
            {
                g.setColour( ui::Theme::padButtonColour( colourId ).withMultipliedAlpha( alpha ) );
                g.fillRoundedRectangle( bounds, 6.0f );
            }

            if( isDown )
            {
                g.setColour( juce::Colours::white.withAlpha( 0.35f ) );
                g.fillRoundedRectangle( bounds, 6.0f );
            }
            g.setColour( isDown ? juce::Colours::white : colours::outline );
            g.drawRoundedRectangle( bounds, 6.0f, isDown ? 2.0f : 1.0f );

            auto textArea = getLocalBounds().reduced( 6, 4 );
            g.setColour( juce::Colours::white.withAlpha( 0.55f ) );
            g.setFont( panelFont( 10.0f ) );
            g.drawText( juce::String( index + 1 ), textArea, juce::Justification::bottomLeft, false );
            g.setColour( juce::Colours::white.withAlpha( 0.8f ) );
            g.setFont( panelFont( 13.0f, true ) );
            g.drawText( hint, textArea, juce::Justification::topRight, false );
        }

        // Lit while held, whether by the mouse or by its key.
        void setPressed( bool pressed )
        {
            if( pressed != isDown )
            {
                isDown = pressed;
                repaint();
            }
        }

        void mouseDown( const juce::MouseEvent& e ) override
        {
            // Strike position sets the velocity: the top edge is the hardest hit.
            velocity = juce::roundToInt( juce::jmap( juce::jlimit( 0.0f, 1.0f, e.position.y / (float)getHeight() ), 127.0f, 50.0f ) );
            sendPad( index, true, velocity );
        }

        void mouseUp( const juce::MouseEvent& ) override { sendPad( index, false, velocity ); }

    private:
        const int   index;
        const bool& blinkPhase;
        int         velocity{ 100 };
        bool        isDown{ false };
    };

    //==============================================================================
    // An endless encoder: a Q-Link knob, or the jog wheel (bigger, with a push centre).
    class ControlPanel::Encoder : public juce::Component
    {
    public:
        Encoder( int knob, juce::String labelText, bool jogWheel )
            : knobId( knob ), label( std::move( labelText ) ), isJog( jogWheel ) {}

        juce::String hint;

        void paint( juce::Graphics& g ) override
        {
            auto area   = getLocalBounds().toFloat();
            auto text   = area.removeFromBottom( 14.0f );
            auto size   = juce::jmin( area.getWidth(), area.getHeight() ) - 4.0f;
            auto circle = juce::Rectangle<float>( size, size ).withCentre( area.getCentre() );

            g.setColour( colours::button );
            g.fillEllipse( circle );
            g.setColour( isActive ? colours::lit : colours::outline );
            g.drawEllipse( circle, isActive ? 2.0f : 1.5f );

            // Position marker: endless, so it only shows movement.
            auto angle  = (float)position * juce::MathConstants<float>::twoPi / ( isJog ? 24.0f : 32.0f );
            auto radius = size * 0.5f;
            auto c      = circle.getCentre();
            auto tip    = c.getPointOnCircumference( radius - 4.0f, angle );
            auto from   = c.getPointOnCircumference( radius * ( isJog ? 0.55f : 0.35f ), angle );
            g.setColour( colours::lit );
            g.drawLine( { from, tip }, isJog ? 3.0f : 2.5f );

            if( isJog )
            {
                auto centre = circle.withSizeKeepingCentre( size * 0.38f, size * 0.38f );
                g.setColour( pushDown ? colours::buttonDown : colours::section );
                g.fillEllipse( centre );
                g.setColour( colours::outline );
                g.drawEllipse( centre, 1.0f );
                g.setColour( colours::hint );
                g.setFont( panelFont( 9.0f, true ) );
                g.drawText( "PUSH", centre, juce::Justification::centred, false );
            }

            g.setColour( colours::text );
            g.setFont( panelFont( 10.0f, true ) );
            g.drawText( hint.isEmpty() ? label : label + "  " + hint, text, juce::Justification::centred, false );
        }

        void mouseDown( const juce::MouseEvent& e ) override
        {
            isActive  = true;
            dragged   = false;
            lastPos   = e.position;
            remainder = 0.0f;
            pushDown  = isJog && isOnPushCentre( e.position );
            repaint();
        }

        void mouseDrag( const juce::MouseEvent& e ) override
        {
            if( isJog )
            {
                // Follow the pointer around the wheel: 24 steps per turn.
                auto c     = getLocalBounds().toFloat().withTrimmedBottom( 14.0f ).getCentre();
                auto delta = c.getAngleToPoint( e.position ) - c.getAngleToPoint( lastPos );
                while( delta > juce::MathConstants<float>::pi ) delta -= juce::MathConstants<float>::twoPi;
                while( delta < -juce::MathConstants<float>::pi ) delta += juce::MathConstants<float>::twoPi;
                if( e.position.getDistanceFrom( c ) > 6.0f )
                    turn( delta * 24.0f / juce::MathConstants<float>::twoPi );
            }
            else
            {
                // Vertical drag, one step every 4 pixels, up is clockwise.
                turn( ( lastPos.y - e.position.y ) / 4.0f );
            }
            lastPos = e.position;
        }

        void mouseUp( const juce::MouseEvent& ) override
        {
            if( pushDown && !dragged )
            {
                sendButton( DCC::EN_KNOB_ENTER, true );
                sendButton( DCC::EN_KNOB_ENTER, false );
            }
            pushDown = false;
            isActive = false;
            repaint();
        }

        void mouseWheelMove( const juce::MouseEvent&, const juce::MouseWheelDetails& wheel ) override
        {
            remainder = 0.0f;
            turn( ( wheel.isReversed ? -wheel.deltaY : wheel.deltaY ) * 8.0f, true );
        }

    private:
        bool isOnPushCentre( juce::Point<float> p ) const
        {
            auto area = getLocalBounds().toFloat().withTrimmedBottom( 14.0f );
            return p.getDistanceFrom( area.getCentre() ) < juce::jmin( area.getWidth(), area.getHeight() ) * 0.19f;
        }

        void turn( float amount, bool atLeastOneStep = false )
        {
            remainder += amount;
            auto steps = (int)remainder; // truncate towards zero, keep the fraction for later
            if( steps == 0 && atLeastOneStep && amount != 0.0f )
                steps = amount > 0.0f ? 1 : -1;
            if( steps == 0 )
                return;

            remainder -= (float)steps;
            dragged = true;
            position += steps;
            sendKnob( knobId, steps );
            repaint();
        }

        const int          knobId;
        const juce::String label;
        const bool         isJog;

        juce::Point<float> lastPos;
        float              remainder{ 0.0f };
        int                position{ 0 };
        bool               isActive{ false }, dragged{ false }, pushDown{ false };
    };

    //==============================================================================
    class ControlPanel::TouchStrip : public juce::Component
    {
    public:
        void paint( juce::Graphics& g ) override
        {
            auto bounds = getLocalBounds().toFloat().reduced( 1.0f );
            g.setColour( colours::button );
            g.fillRoundedRectangle( bounds, 4.0f );

            // LED bar the app drives, plus the finger while touching.
            auto level = (float)HardwareState::get().getSliderLevel() / 100.0f;
            g.setColour( colours::lit.withAlpha( 0.55f ) );
            g.fillRoundedRectangle( bounds.withWidth( bounds.getWidth() * level ), 4.0f );

            g.setColour( colours::outline );
            for( int i = 1; i < 16; ++i )
            {
                auto x = bounds.getX() + bounds.getWidth() * (float)i / 16.0f;
                g.drawVerticalLine( juce::roundToInt( x ), bounds.getY() + 4.0f, bounds.getBottom() - 4.0f );
            }

            if( touching )
            {
                auto x = bounds.getX() + bounds.getWidth() * (float)lastPercent / 100.0f;
                g.setColour( juce::Colours::white );
                g.fillRoundedRectangle( juce::Rectangle<float>( 4.0f, bounds.getHeight() ).withCentre( { x, bounds.getCentreY() } ), 2.0f );
            }
            g.setColour( colours::outline );
            g.drawRoundedRectangle( bounds, 4.0f, 1.0f );
            g.setColour( colours::hint );
            g.setFont( panelFont( 9.0f, true ) );
            g.drawText( "TOUCH STRIP", bounds.reduced( 6.0f, 0.0f ), juce::Justification::centredLeft, false );
        }

        void mouseDown( const juce::MouseEvent& e ) override
        {
            touching = true;
            update( e );
        }

        void mouseDrag( const juce::MouseEvent& e ) override { update( e ); }

        void mouseUp( const juce::MouseEvent& ) override
        {
            touching = false;
            repaint();
        }

    private:
        void update( const juce::MouseEvent& e )
        {
            auto percent = juce::roundToInt( juce::jlimit( 0.0f, 1.0f, e.position.x / (float)getWidth() ) * 100.0f );
            if( percent != lastPercent || !sentOnce )
            {
                lastPercent = percent;
                sentOnce    = true;
                sendSlider( percent );
            }
            repaint();
        }

        int  lastPercent{ 0 };
        bool touching{ false }, sentOnce{ false };
    };

    //==============================================================================
    namespace
    {
        struct NamedButton
        {
            int         id;
            const char* name;
        };

        const NamedButton modeButtons[] = {
            { DCC::EN_MAINMODE, "Main" }, { DCC::EN_GRID, "Grid" }, { DCC::EN_STEP_SEQ, "Step Seq" },
            { DCC::EN_BROWSE, "Browse" }, { DCC::EN_SAMPLER, "Sampler" }, { DCC::EN_SAMPLE_EDIT, "Sample Edit" },
            { DCC::EN_PROG_EDIT, "Program Edit" }, { DCC::EN_LOOPER, "Looper" }, { DCC::EN_FX, "XY FX" },
            { DCC::EN_AUTOMATION, "Automation" }, { DCC::EN_TC, "Time Correct" }, { DCC::EN_MIDI_CONTROL, "MIDI Control" },
            { DCC::EN_LINK_EDIT, "Link Edit" },
        };

        const NamedButton mixButtons[] = {
            { DCC::EN_TRACK_MIX, "Track Mix" }, { DCC::EN_PAD_MIX, "Pad Mix" }, { DCC::EN_TRACK_MUTE, "Track Mute" },
            { DCC::EN_PAD_MUTE, "Pad Mute" }, { DCC::EN_NEXT_SEQ, "Next Seq" }, { DCC::EN_MASTER, "Master" },
        };

        const NamedButton editButtons[] = {
            { DCC::EN_MENU, "Menu" }, { DCC::EN_PREF, "Preferences" }, { DCC::EN_SAVE, "Save" },
            { DCC::EN_BACK, "Back / Undo" }, { DCC::EN_COPY, "Copy" }, { DCC::EN_ERASE, "Erase" },
            { DCC::EN_PLUS, "+" }, { DCC::EN_MINUS, "-" }, { DCC::EN_TAP_TEMPO, "Tap Tempo" },
            { DCC::EN_ON_OFF, "On / Off" }, { DCC::EN_LINK, "Link" }, { DCC::EN_NOTES, "Notes" },
            { DCC::EN_LATCH, "Latch" }, { DCC::EN_HALF_LEVEL, "Half Level" },
        };
    }

    ControlPanel::ControlPanel()
    {
        setOpaque( true );

        for( int i = 0; i < 4; ++i )
        {
            knobs.push_back( std::make_unique<Encoder>( DCC::EN_KNOB_1 + i, "Q" + juce::String( i + 1 ), false ) );
            addAndMakeVisible( *knobs.back() );
        }
        jog = std::make_unique<Encoder>( DCC::EN_KNOB_CENTER, "JOG", true );
        addAndMakeVisible( *jog );

        strip = std::make_unique<TouchStrip>();
        addAndMakeVisible( *strip );

        for( int b = 0; b < 8; ++b )
        {
            banks.push_back( std::make_unique<PanelButton>( juce::String::charToString( (juce::juce_wchar)( 'A' + b ) ), DCC::EN_BANK_A + b ) );
            addAndMakeVisible( *banks.back() );
        }

        for( auto [id, name] : { NamedButton{ DCC::EN_FULL_LEVEL, "FULL LEVEL" }, NamedButton{ DCC::EN_16LEVELS, "16 LEVELS" },
                                 NamedButton{ DCC::EN_NOTE_REPEAT, "NOTE REPEAT" }, NamedButton{ DCC::EN_ERASE, "ERASE" },
                                 NamedButton{ DCC::EN_SHIFT, "SHIFT" } } )
        {
            sideButtons.push_back( std::make_unique<PanelButton>( name, id ) );
            addAndMakeVisible( *sideButtons.back() );
        }

        // Pads are numbered from the bottom-left, as on the hardware.
        for( int p = 0; p < HardwareState::numPads; ++p )
        {
            pads.push_back( std::make_unique<PadButton>( p, blinkPhase ) );
            addAndMakeVisible( *pads.back() );
        }

        for( auto [id, name] : { NamedButton{ DCC::EN_RECORD, "REC" }, NamedButton{ DCC::EN_OVERDUB, "OVERDUB" },
                                 NamedButton{ DCC::EN_STOP, "STOP" }, NamedButton{ DCC::EN_PLAY, "PLAY" },
                                 NamedButton{ DCC::EN_PLAYSTART, "PLAY START" } } )
        {
            transport.push_back( std::make_unique<PanelButton>( name, id ) );
            addAndMakeVisible( *transport.back() );
        }

        modes          = std::make_unique<PanelButton>( "MODES", DCC::EN_INVALID_ID );
        modes->onClick = [this] { showModesMenu(); };
        addAndMakeVisible( *modes );

        HardwareState::get().onChange = [this] { refresh(); };

        // Show presses from every source (mouse, keyboard) the way a finger on the panel would.
        inputConnection = contracts::KeyboardSignals::KeyboardEventTriggered.connect( [this]( CPktMgr::HWMsg_t& msg ) {
            if( msg.data.size() < 2 )
                return;
            auto pressed = msg.data[1] == DCC::BTN_PRESSED;
            if( msg.pktType == CPktMgr::EN_PKT_TYPE_PAD && juce::isPositiveAndBelow( msg.data[0], (int)pads.size() ) )
                pads[(size_t)msg.data[0]]->setPressed( pressed );
            else if( msg.pktType == CPktMgr::EN_PKT_TYPE_BUTTON )
                for( auto* group : { &banks, &sideButtons, &transport } )
                    for( auto& b : *group )
                        if( b->buttonId == msg.data[0] )
                            b->setPressed( pressed );
        } );

        startTimer( 250 );
    }

    ControlPanel::~ControlPanel()
    {
        inputConnection.disconnect();
        HardwareState::get().onChange = nullptr;
    }

    void ControlPanel::setKeyMap( const KeyMap& map )
    {
        keyMap = map;

        for( int p = 0; p < (int)pads.size(); ++p )
            pads[(size_t)p]->hint = keyLabelForPad( map, p );
        for( auto* group : { &banks, &sideButtons, &transport } )
            for( auto& b : *group )
                b->hint = keyLabelForButton( map, b->buttonId );
        jog->hint = "Up/Down";
        repaint();
    }

    void ControlPanel::refresh()
    {
        // Everything here is cheap to paint; repaint the panel in one go.
        repaint();
    }

    void ControlPanel::timerCallback()
    {
        blinkPhase = !blinkPhase;
        for( auto& pad : pads )
            if( pad->isBlinking() )
                pad->repaint();
    }

    void ControlPanel::showModesMenu()
    {
        juce::PopupMenu menu;
        auto addSection = [this, &menu]( const char* title, const auto& buttons ) {
            menu.addSectionHeader( title );
            for( const auto& b : buttons )
            {
                juce::PopupMenu::Item item( b.name );
                item.shortcutKeyDescription = keyLabelForButton( keyMap, b.id );
                item.action                 = [id = b.id] {
                    sendButton( id, true );
                    sendButton( id, false );
                };
                menu.addItem( std::move( item ) );
            }
        };
        addSection( "Modes", modeButtons );
        addSection( "Mix and mute", mixButtons );
        addSection( "Edit", editButtons );

        if( addWindowItems )
        {
            menu.addSectionHeader( "Window" );
            addWindowItems( menu );
        }

        menu.showMenuAsync( juce::PopupMenu::Options().withTargetComponent( modes.get() ) );
    }

    void ControlPanel::paint( juce::Graphics& g )
    {
        g.fillAll( colours::background );
        g.setColour( colours::section );
        g.fillRoundedRectangle( getLocalBounds().toFloat().reduced( 4.0f ), 8.0f );
    }

    void ControlPanel::resized()
    {
        auto area = getLocalBounds().reduced( 12 );

        // Q-Link knobs and the jog wheel.
        auto top     = area.removeFromTop( 110 );
        auto jogArea = top.removeFromRight( 120 );
        jog->setBounds( jogArea );
        auto knobW = top.getWidth() / 4;
        for( auto& k : knobs )
            k->setBounds( top.removeFromLeft( knobW ).withSizeKeepingCentre( knobW, 76 ) );

        area.removeFromTop( 8 );
        strip->setBounds( area.removeFromTop( 32 ) );
        area.removeFromTop( 8 );

        auto bankRow = area.removeFromTop( 30 );
        auto bankW   = ( bankRow.getWidth() - 7 * 4 ) / 8;
        for( auto& b : banks )
        {
            b->setBounds( bankRow.removeFromLeft( bankW ) );
            bankRow.removeFromLeft( 4 );
        }
        area.removeFromTop( 8 );

        auto transportRow = area.removeFromBottom( 32 );
        area.removeFromBottom( 8 );

        // Pads with the performance buttons in a column on their right.
        auto side = area.removeFromRight( 78 );
        area.removeFromRight( 6 );
        auto sideH = ( side.getHeight() - 4 * 6 ) / 5;
        for( auto& b : sideButtons )
        {
            b->setBounds( side.removeFromTop( sideH ) );
            side.removeFromTop( 6 );
        }

        auto padSize = juce::jmin( ( area.getWidth() - 3 * 6 ) / 4, ( area.getHeight() - 3 * 6 ) / 4 );
        auto grid    = area.withSizeKeepingCentre( padSize * 4 + 18, padSize * 4 + 18 );
        for( int p = 0; p < (int)pads.size(); ++p )
        {
            int row = 3 - p / 4, col = p % 4;
            pads[(size_t)p]->setBounds( grid.getX() + col * ( padSize + 6 ), grid.getY() + row * ( padSize + 6 ), padSize, padSize );
        }

        auto buttonW = ( transportRow.getWidth() - 5 * 4 ) / 6;
        for( auto& b : transport )
        {
            b->setBounds( transportRow.removeFromLeft( buttonW ) );
            transportRow.removeFromLeft( 4 );
        }
        modes->setBounds( transportRow );
    }
}
