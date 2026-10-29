/*
 * This file is part of the Linux desktop port of Kick.
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

namespace desktop
{
    namespace
    {
        juce::File audioSettingsFile() { return getAppDirs( MTPrefs ).getChildFile( "audio-device.xml" ); }

        // Saves the device setup when the dialog goes away, however it was closed.
        class AudioSettingsContent : public juce::Component
        {
        public:
            explicit AudioSettingsContent( juce::AudioDeviceManager& dm )
                : deviceManager( dm ),
                  selector( dm, 0, AUDIO_NUM_INPUTS, 0, AUDIO_NUM_OUTPUTS, false, false, true, false )
            {
                addAndMakeVisible( selector );
                note.setText( "The engine runs at " + juce::String( (int)AUDIO_SAMPLE_RATE ) + " Hz; other rates are switched back to it.",
                              juce::dontSendNotification );
                note.setColour( juce::Label::textColourId, juce::Colours::grey );
                addAndMakeVisible( note );
                setSize( 520, 420 );
            }

            ~AudioSettingsContent() override
            {
                if( auto state = deviceManager.createStateXml() )
                    state->writeTo( audioSettingsFile() );
            }

            void resized() override
            {
                auto area = getLocalBounds().reduced( 8 );
                note.setBounds( area.removeFromBottom( 24 ) );
                selector.setBounds( area );
            }

        private:
            juce::AudioDeviceManager&            deviceManager;
            juce::AudioDeviceSelectorComponent selector;
            juce::Label                        note;
        };
    }

    void restoreAudioSettings()
    {
        mt::AudioDevice::loadSavedState = []() -> std::unique_ptr<juce::XmlElement> {
            auto file = audioSettingsFile();
            return file.existsAsFile() ? juce::parseXML( file ) : nullptr;
        };
    }

    void showAudioSettings()
    {
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned( new AudioSettingsContent( mt::getAudioInterface().getAudioDeviceManager() ) );
        options.dialogTitle                  = "Audio settings";
        options.dialogBackgroundColour       = juce::Colour( 0xff1a1a1a );
        options.useNativeTitleBar            = true;
        options.resizable                    = false;
        options.escapeKeyTriggersCloseButton = true;
        options.launchAsync();
    }
}
