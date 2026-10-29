// mt::AudioDevice, reconstructed: AudIO's engine driven by a JUCE audio device.
//
// MTAudioManager makes one with ( ins, outs, deviceName ), registers it with
// DawEngine::addAudioDevice and later asks it for getJuceAudioDeviceMgr(). AudIO's
// IAudioDevice defines the contract (start / resume / pause / stop, and a Buffer<float>
// handed to the engine's callback); this fills it in with juce::AudioDeviceManager.
//
// The engine works in fixed blocks of Buffer::getBlockSize() at a fixed sample rate. A device
// callback that is a whole number of blocks is processed in place. Any other size (JACK or
// PipeWire at 128 frames, odd ALSA periods) goes through FIFOs so the engine still sees whole
// blocks and keeps time, at the cost of up to one block of latency. The device is kept at the
// engine's sample rate when the user picks another device.

#pragma once

#include <numeric>

namespace mt
{
    class AudioDevice
        : public AudIO::IAudioDevice
        , private juce::AudioIODeviceCallback
        , private juce::ChangeListener
    {
    public:
        AudioDevice( uint32_t nInputs, uint32_t nOutputs, const juce::String& deviceName )
            : AudIO::IAudioDevice( nInputs, nOutputs ), numIns( (int)nInputs ), numOuts( (int)nOutputs ), name( deviceName )
        {
        }

        ~AudioDevice() override { stop(); }

        // Device settings the application saved earlier (AudioDeviceManager::createStateXml()),
        // restored when the device starts. Optional.
        inline static std::function<std::unique_ptr<juce::XmlElement>()> loadSavedState;

        juce::AudioDeviceManager& getJuceAudioDeviceMgr() noexcept { return deviceManager; }

        void start( uint32_t samplerate, uint32_t blocksize ) override
        {
            engineSampleRate = (double)samplerate;

            auto saved = loadSavedState ? loadSavedState() : nullptr;
            deviceManager.initialise( numIns, numOuts, saved.get(), true, name, nullptr );

            juce::AudioDeviceManager::AudioDeviceSetup setup;
            deviceManager.getAudioDeviceSetup( setup );
            setup.sampleRate = engineSampleRate;
            if( saved == nullptr )
                setup.bufferSize = (int)blocksize;
            deviceManager.setAudioDeviceSetup( setup, true );

            prepareToPlay( samplerate, blocksize );
            deviceManager.addAudioCallback( this );
            deviceManager.addChangeListener( this );
            setState( AudIO::AudioDeviceState::playing );
        }

        void resume() override { setState( AudIO::AudioDeviceState::playing ); }
        void pause() override { setState( AudIO::AudioDeviceState::paused ); }

        void stop() override
        {
            if( !isInitialized() )
                return;
            deviceManager.removeChangeListener( this );
            deviceManager.removeAudioCallback( this );
            deviceManager.closeAudioDevice();
            teardown();
            setState( AudIO::AudioDeviceState::stopped );
        }

    private:
        //==============================================================================
        void audioDeviceIOCallbackWithContext( const float* const* inputs, int numInputChannels, float* const* outputs,
                                               int numOutputChannels, int numSamples,
                                               const juce::AudioIODeviceCallbackContext& ) override
        {
            auto&     buf   = getBuffer();
            const int block = (int)buf.getBlockSize();

            if( !isPlaying() || block <= 0 )
            {
                clearOutputs( outputs, numOutputChannels, 0, numSamples );
                return;
            }

            // Whole blocks and nothing queued: process in place, no added latency.
            if( numSamples % block == 0 && inFifo.getNumReady() == 0 && outFifo.getNumReady() == 0 )
            {
                for( int pos = 0; pos < numSamples; pos += block )
                {
                    for( int ch = 0; ch < (int)buf.getNumInputChannels(); ++ch )
                        if( auto in = buf.getInputBuffer( (uint32_t)ch ) )
                        {
                            if( ch < numInputChannels && inputs[ch] != nullptr )
                                juce::FloatVectorOperations::copy( in, inputs[ch] + pos, block );
                            else
                                juce::FloatVectorOperations::clear( in, block );
                        }

                    buf.clearOutputs();
                    process( buf );

                    for( int ch = 0; ch < numOutputChannels; ++ch )
                        if( outputs[ch] != nullptr )
                        {
                            auto out = ch < (int)buf.getNumOutputChannels() ? buf.getOutputBuffer( (uint32_t)ch ) : nullptr;
                            if( out != nullptr )
                                juce::FloatVectorOperations::copy( outputs[ch] + pos, out, block );
                            else
                                juce::FloatVectorOperations::clear( outputs[ch] + pos, block );
                        }
                }
                return;
            }

            // Any other size: queue the input, run the engine on each full block, play back
            // what is queued.
            pushInput( inputs, numInputChannels, numSamples );

            while( inFifo.getNumReady() >= block && outFifo.getFreeSpace() >= block )
            {
                popInputBlock( buf, block );
                buf.clearOutputs();
                process( buf );
                pushOutputBlock( buf, block );
            }

            popOutput( outputs, numOutputChannels, numSamples );
        }

        void audioDeviceAboutToStart( juce::AudioIODevice* device ) override
        {
            // Called before the callback (re)starts, never during it: allocate here.
            auto&     buf      = getBuffer();
            const int block    = juce::jmax( 1, (int)buf.getBlockSize() );
            const int capacity = juce::jmax( 1, device->getCurrentBufferSizeSamples() ) + 2 * block + 1;

            inData.setSize( juce::jmax( 1, (int)buf.getNumInputChannels() ), capacity );
            outData.setSize( juce::jmax( 1, (int)buf.getNumOutputChannels() ), capacity );
            inData.clear();
            outData.clear();
            inFifo.setTotalSize( capacity );
            outFifo.setTotalSize( capacity );
            inFifo.reset();
            outFifo.reset();

            // Start the output queue with the least silence that keeps it from ever running dry:
            // after k callbacks of n samples, whole blocks cover all but (k * n) mod block of
            // them, which is at most block - gcd( n, block ). Zero when n is a multiple of block.
            const int deviceBlock = juce::jmax( 1, device->getCurrentBufferSizeSamples() );
            const int preroll     = block - std::gcd( deviceBlock, block );
            int       s1, n1, s2, n2;
            outFifo.prepareToWrite( preroll, s1, n1, s2, n2 );
            outFifo.finishedWrite( n1 + n2 ); // the buffer was cleared above
        }

        void audioDeviceStopped() override {}

        // The engine's tempo and pitch assume its own sample rate: after a device change,
        // bring the device back to it if it can run at that rate.
        void changeListenerCallback( juce::ChangeBroadcaster* ) override
        {
            auto* device = deviceManager.getCurrentAudioDevice();
            if( device == nullptr || device->getCurrentSampleRate() == engineSampleRate )
                return;

            if( device->getAvailableSampleRates().contains( engineSampleRate ) )
            {
                juce::AudioDeviceManager::AudioDeviceSetup setup;
                deviceManager.getAudioDeviceSetup( setup );
                setup.sampleRate = engineSampleRate;
                deviceManager.setAudioDeviceSetup( setup, true );
            }
            else
            {
                juce::Logger::writeToLog( "Audio device cannot run at " + juce::String( engineSampleRate ) + " Hz: tempo and pitch will be off" );
            }
        }

        //==============================================================================
        static void clearOutputs( float* const* outputs, int numChannels, int start, int num )
        {
            for( int ch = 0; ch < numChannels; ++ch )
                if( outputs[ch] != nullptr )
                    juce::FloatVectorOperations::clear( outputs[ch] + start, num );
        }

        void pushInput( const float* const* inputs, int numInputChannels, int numSamples )
        {
            int s1, n1, s2, n2;
            inFifo.prepareToWrite( numSamples, s1, n1, s2, n2 );
            for( int ch = 0; ch < inData.getNumChannels(); ++ch )
            {
                auto src = ch < numInputChannels ? inputs[ch] : nullptr;
                auto dst = inData.getWritePointer( ch );
                if( src != nullptr )
                {
                    juce::FloatVectorOperations::copy( dst + s1, src, n1 );
                    juce::FloatVectorOperations::copy( dst + s2, src + n1, n2 );
                }
                else
                {
                    juce::FloatVectorOperations::clear( dst + s1, n1 );
                    juce::FloatVectorOperations::clear( dst + s2, n2 );
                }
            }
            inFifo.finishedWrite( n1 + n2 );
        }

        void popInputBlock( AudIO::Buffer<float>& buf, int block )
        {
            int s1, n1, s2, n2;
            inFifo.prepareToRead( block, s1, n1, s2, n2 );
            for( int ch = 0; ch < (int)buf.getNumInputChannels(); ++ch )
                if( auto in = buf.getInputBuffer( (uint32_t)ch ) )
                {
                    auto src = inData.getReadPointer( juce::jmin( ch, inData.getNumChannels() - 1 ) );
                    juce::FloatVectorOperations::copy( in, src + s1, n1 );
                    juce::FloatVectorOperations::copy( in + n1, src + s2, n2 );
                }
            inFifo.finishedRead( n1 + n2 );
        }

        void pushOutputBlock( AudIO::Buffer<float>& buf, int block )
        {
            int s1, n1, s2, n2;
            outFifo.prepareToWrite( block, s1, n1, s2, n2 );
            for( int ch = 0; ch < outData.getNumChannels(); ++ch )
            {
                auto dst = outData.getWritePointer( ch );
                auto out = ch < (int)buf.getNumOutputChannels() ? buf.getOutputBuffer( (uint32_t)ch ) : nullptr;
                if( out != nullptr )
                {
                    juce::FloatVectorOperations::copy( dst + s1, out, n1 );
                    juce::FloatVectorOperations::copy( dst + s2, out + n1, n2 );
                }
                else
                {
                    juce::FloatVectorOperations::clear( dst + s1, n1 );
                    juce::FloatVectorOperations::clear( dst + s2, n2 );
                }
            }
            outFifo.finishedWrite( n1 + n2 );
        }

        void popOutput( float* const* outputs, int numOutputChannels, int numSamples )
        {
            int s1, n1, s2, n2;
            outFifo.prepareToRead( numSamples, s1, n1, s2, n2 );
            for( int ch = 0; ch < numOutputChannels; ++ch )
            {
                if( outputs[ch] == nullptr )
                    continue;
                if( ch < outData.getNumChannels() )
                {
                    auto src = outData.getReadPointer( ch );
                    juce::FloatVectorOperations::copy( outputs[ch], src + s1, n1 );
                    juce::FloatVectorOperations::copy( outputs[ch] + n1, src + s2, n2 );
                }
                else
                {
                    juce::FloatVectorOperations::clear( outputs[ch], n1 + n2 );
                }
            }
            outFifo.finishedRead( n1 + n2 );

            // Only while the queue fills up after a (re)start.
            if( n1 + n2 < numSamples )
                clearOutputs( outputs, numOutputChannels, n1 + n2, numSamples - ( n1 + n2 ) );
        }

        //==============================================================================
        juce::AudioDeviceManager deviceManager;
        int                      numIns, numOuts;
        juce::String             name;
        double                   engineSampleRate{ AUDIO_SAMPLE_RATE };

        juce::AbstractFifo         inFifo{ 1 }, outFifo{ 1 };
        juce::AudioBuffer<float>   inData, outData;
    };
} // namespace mt
