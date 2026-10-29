/*
 * This file is part of the Linux desktop port of Kick.
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

namespace desktop
{
    namespace
    {
        const float zoomLevels[] = { 0.75f, 0.9f, 1.0f, 1.1f, 1.2f, 1.3f, 1.5f, 1.75f, 2.0f, 2.5f, 3.0f };

        float nextZoom( float current, int direction )
        {
            if( direction > 0 )
            {
                for( auto z : zoomLevels )
                    if( z > current + 0.01f ) return z;
                return zoomLevels[std::size( zoomLevels ) - 1];
            }
            for( auto it = std::rbegin( zoomLevels ); it != std::rend( zoomLevels ); ++it )
                if( *it < current - 0.01f ) return *it;
            return zoomLevels[0];
        }

        // The largest zoom level whose window fits in most of the work area.
        float defaultZoom( juce::Rectangle<int> contentSize )
        {
            auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
            if( display == nullptr )
                return 1.0f;

            auto scale = juce::Desktop::getInstance().getGlobalScaleFactor();
            auto area  = display->userArea.toFloat() * scale; // in zoom-1 units
            auto fit   = juce::jmin( area.getWidth() * 0.9f / (float)contentSize.getWidth(), area.getHeight() * 0.85f / (float)contentSize.getHeight() );

            float best = zoomLevels[0];
            for( auto z : zoomLevels )
                if( z <= fit ) best = z;
            return best;
        }
    }

    //==============================================================================
    Shell::Shell( juce::Component& appScreen )
        : screen( appScreen )
    {
        setOpaque( true );
        addAndMakeVisible( screen );
        addAndMakeVisible( panel );
    }

    Shell::~Shell()
    {
        removeChildComponent( &screen );
    }

    void Shell::setPanelVisible( bool shouldBeVisible )
    {
        panel.setVisible( shouldBeVisible );
        resized();
    }

    juce::Rectangle<int> Shell::getContentSize() const
    {
        return { ui::layout::getAppWidth() + ( panel.isVisible() ? ControlPanel::width : 0 ), ui::layout::getAppHeight() };
    }

    void Shell::paint( juce::Graphics& g )
    {
        g.fillAll( juce::Colours::black );
    }

    void Shell::resized()
    {
        auto content = getContentSize().withCentre( getLocalBounds().getCentre() );
        content.setPosition( juce::jmax( 0, content.getX() ), juce::jmax( 0, content.getY() ) );

        screen.setBounds( content.removeFromLeft( ui::layout::getAppWidth() ) );
        if( panel.isVisible() )
            panel.setBounds( content.removeFromLeft( ControlPanel::width ) );
    }

    //==============================================================================
    MainWindow::MainWindow( const juce::String& name, juce::Component& appScreen )
        : juce::DocumentWindow( name, juce::Colours::black, juce::DocumentWindow::closeButton | juce::DocumentWindow::minimiseButton )
    {
        juce::PropertiesFile::Options options;
        options.storageFormat            = juce::PropertiesFile::storeAsXML;
        options.millisecondsBeforeSaving = 500;
        settings = std::make_unique<juce::PropertiesFile>( getAppDirs( MTPrefs ).getChildFile( "desktop-window.settings" ), options );

        setUsingNativeTitleBar( true );
        setResizable( false, false );
        setTitle( name );

        shell = new Shell( appScreen );
        shell->setPanelVisible( settings->getBoolValue( "panel", true ) );
        setContentOwned( shell, false );

        // Keys reach the panel key map whichever component has focus, including none.
        auto& keys           = contracts::getKeypressDetector();
        keys.padVelocity     = 100;
        keys.shortcutHandler = [this]( const juce::KeyPress& key ) { return handleShortcut( key ); };
        keys.setKeyMap( buildKeyMap() );
        shell->getPanel().setKeyMap( keys.getKeyMap() );
        shell->getPanel().addWindowItems = [this]( juce::PopupMenu& menu ) { addWindowItems( menu ); };
        addKeyListener( &keys );

        // Popups that cover the app screen must line up with it inside this window.
        mt::getDesktopScreenArea = [this] { return shell != nullptr ? shell->getAppScreenArea() : juce::Rectangle<int>(); };

        zoom = (float)settings->getDoubleValue( "zoom", defaultZoom( shell->getContentSize() ) );
        setZoom( zoom );
        centreWithSize( getWidth(), getHeight() );
        setVisible( true );

        if( settings->getBoolValue( "fullScreen", false ) )
            toggleFullScreen();
    }

    MainWindow::~MainWindow()
    {
        mt::getDesktopScreenArea = nullptr;

        auto& keys           = contracts::getKeypressDetector();
        keys.shortcutHandler = nullptr;
        keys.releaseAllKeys();
        removeKeyListener( &keys );

        if( fullScreen )
            juce::Desktop::getInstance().setKioskModeComponent( nullptr, false );
        saveSettings();
        settings->saveIfNeeded();
        clearContentComponent(); // deletes the shell, which gives the app screen back
        shell = nullptr;
    }

    void MainWindow::closeButtonPressed()
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

    void MainWindow::inputAttemptWhenModal()
    {
        auto numModal = getNumCurrentlyModalComponents();
        auto index    = juce::jmax( 0, numModal - 1 );
        if( auto p = getCurrentlyModalComponent( index ) )
            p->toFront( true );
    }

    void MainWindow::activeWindowStatusChanged()
    {
        // A key released while another window has focus never reaches us: let go of
        // everything rather than leave a pad or SHIFT held down.
        if( !isActiveWindow() )
            contracts::getKeypressDetector().releaseAllKeys();
    }

    void MainWindow::applyLayout()
    {
        auto& desktopInstance = juce::Desktop::getInstance();
        if( !fullScreen )
        {
            desktopInstance.setGlobalScaleFactor( zoom );
            setContentComponentSize( shell->getContentSize().getWidth(), shell->getContentSize().getHeight() );
            return;
        }

        // Largest scale at which the content fits the whole display.
        auto  scale   = desktopInstance.getGlobalScaleFactor();
        auto* display = desktopInstance.getDisplays().getDisplayForRect( getScreenBounds() );
        auto  area    = ( display != nullptr ? display->totalArea : getScreenBounds() ).toFloat() * scale;
        auto  content = shell->getContentSize().toFloat();
        desktopInstance.setGlobalScaleFactor( juce::jmin( area.getWidth() / content.getWidth(), area.getHeight() / content.getHeight() ) );
        desktopInstance.setKioskModeComponent( this, false );
        shell->setBounds( getLocalBounds() );
    }

    void MainWindow::setZoom( float newZoom )
    {
        zoom = juce::jlimit( zoomLevels[0], zoomLevels[std::size( zoomLevels ) - 1], newZoom );
        applyLayout();
        saveSettings();
    }

    void MainWindow::toggleFullScreen()
    {
        fullScreen = !fullScreen;
        if( !fullScreen )
            juce::Desktop::getInstance().setKioskModeComponent( nullptr, false );
        applyLayout();
        if( !fullScreen )
            centreWithSize( getWidth(), getHeight() );
        saveSettings();
    }

    void MainWindow::togglePanel()
    {
        shell->setPanelVisible( !shell->isPanelVisible() );
        applyLayout();
        saveSettings();
    }

    bool MainWindow::handleShortcut( const juce::KeyPress& key )
    {
        auto mods   = key.getModifiers();
        auto code   = key.getKeyCode();
        auto letter = juce::CharacterFunctions::toUpperCase( (juce::juce_wchar)code );

        if( mods.isAltDown() && code == juce::KeyPress::returnKey )
        {
            toggleFullScreen();
            return true;
        }
        if( !mods.isCtrlDown() )
            return false;

        if( code == '=' || code == '+' || code == juce::KeyPress::numberPadAdd )
            setZoom( nextZoom( zoom, 1 ) );
        else if( code == '-' || code == juce::KeyPress::numberPadSubtract )
            setZoom( nextZoom( zoom, -1 ) );
        else if( code == '0' || code == juce::KeyPress::numberPad0 )
            setZoom( defaultZoom( shell->getContentSize() ) );
        else if( letter == 'F' )
            toggleFullScreen();
        else if( letter == 'P' )
            togglePanel();
        else if( letter == 'Q' )
            closeButtonPressed();
        else
            return false;
        return true;
    }

    void MainWindow::addWindowItems( juce::PopupMenu& menu )
    {
        auto add = [&menu]( const char* text, const char* keys, std::function<void()> action, bool ticked = false ) {
            juce::PopupMenu::Item item( text );
            item.shortcutKeyDescription = keys;
            item.action                 = std::move( action );
            item.isTicked               = ticked;
            menu.addItem( std::move( item ) );
        };
        add( "Zoom in", "Ctrl +", [this] { setZoom( nextZoom( zoom, 1 ) ); } );
        add( "Zoom out", "Ctrl -", [this] { setZoom( nextZoom( zoom, -1 ) ); } );
        add( "Default zoom", "Ctrl 0", [this] { setZoom( defaultZoom( shell->getContentSize() ) ); } );
        add( "Full screen", "Ctrl F", [this] { toggleFullScreen(); }, fullScreen );
        add( "Show control panel", "Ctrl P", [this] { togglePanel(); }, shell->isPanelVisible() );
        add( "Quit", "Ctrl Q", [this] { closeButtonPressed(); } );
    }

    void MainWindow::saveSettings()
    {
        settings->setValue( "zoom", zoom );
        settings->setValue( "fullScreen", fullScreen );
        settings->setValue( "panel", shell->isPanelVisible() );
    }
}
