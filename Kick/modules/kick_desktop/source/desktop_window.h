/*
 * This file is part of the Linux desktop port of Kick.
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

namespace desktop
{
    // Places the app screen (fixed 1024x600 layout) and the control panel side by side,
    // centred when the window is larger (full screen).
    class Shell : public juce::Component
    {
    public:
        explicit Shell( juce::Component& appScreen );
        ~Shell() override;

        void setPanelVisible( bool shouldBeVisible );
        bool isPanelVisible() const { return panel.isVisible(); }

        // Logical size of the content at zoom 1.
        juce::Rectangle<int> getContentSize() const;
        // The app screen, which hosts the popups that cover it.
        juce::Component& getAppScreen() const { return screen; }

        ControlPanel& getPanel() { return panel; }

        void paint( juce::Graphics& g ) override;
        void resized() override;

    private:
        juce::Component& screen; // owned by the app, never deleted here
        ControlPanel     panel;
    };

    // The application window: native title bar, discrete zoom levels, full screen, and the
    // keyboard wiring. Zoom, full screen and panel visibility are remembered.
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow( const juce::String& name, juce::Component& appScreen );
        ~MainWindow() override;

        void closeButtonPressed() override;
        void inputAttemptWhenModal() override;
        void activeWindowStatusChanged() override;

        void setZoom( float newZoom );
        void toggleFullScreen();
        void togglePanel();

    private:
        bool handleShortcut( const juce::KeyPress& key );
        void applyLayout();
        void saveSettings();
        void addWindowItems( juce::PopupMenu& menu );

        std::unique_ptr<juce::PropertiesFile> settings;
        Shell*                                shell{ nullptr };
        float                                 zoom{ 1.0f };
        bool                                  fullScreen{ false };
    };
}
