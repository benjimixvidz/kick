// Feeds mt::AudioDevice's callback with device block sizes that are and are not multiples of
// the engine block, and checks the engine output arrives as one unbroken ramp.
#include <AudIO_juce/AudIO_juce.h>
#include <cstdio>

static constexpr int engineBlock = AUDIO_BLOCK_SIZE;
static float pool[64 * engineBlock];

struct Ramp : AudIO::IProcessable
{
    float next = 1.0f; // never 0, so the start-up silence is told apart
    long  processed = 0;
    void prepareToPlay( uint32_t, uint32_t ) override {}
    void process( AudIO::Buffer<float>& b ) override
    {
        for( uint32_t i = 0; i < b.getBlockSize(); ++i, next += 1.0f )
            for( uint32_t ch = 0; ch < b.getNumOutputChannels(); ++ch )
                b.getOutputBuffer( ch )[i] = next;
        processed += b.getBlockSize();
    }
};

struct FakeDevice : juce::AudioIODevice
{
    int bs;
    explicit FakeDevice( int b ) : juce::AudioIODevice( "fake", "fake" ), bs( b ) {}
    juce::StringArray getOutputChannelNames() override { return { "L", "R" }; }
    juce::StringArray getInputChannelNames() override { return { "L", "R" }; }
    juce::Array<double> getAvailableSampleRates() override { return { 48000.0 }; }
    juce::Array<int> getAvailableBufferSizes() override { return { bs }; }
    int getDefaultBufferSize() override { return bs; }
    juce::String open( const juce::BigInteger&, const juce::BigInteger&, double, int ) override { return {}; }
    void close() override {}
    bool isOpen() override { return true; }
    void start( juce::AudioIODeviceCallback* ) override {}
    void stop() override {}
    bool isPlaying() override { return true; }
    juce::String getLastError() override { return {}; }
    int getCurrentBufferSizeSamples() override { return bs; }
    double getCurrentSampleRate() override { return 48000.0; }
    int getCurrentBitDepth() override { return 32; }
    juce::BigInteger getActiveOutputChannels() const override { return 3; }
    juce::BigInteger getActiveInputChannels() const override { return 3; }
    int getOutputLatencyInSamples() override { return 0; }
    int getInputLatencyInSamples() override { return 0; }
};

static bool run( int deviceBlock, int callbacks )
{
    Ramp ramp;
    mt::AudioDevice dev( 2, 2, {} );
    dev.setCallback( &ramp );
    dev.setState( AudIO::AudioDeviceState::playing );
    auto* cb = (juce::AudioIODeviceCallback*)&dev; // private base: test access only

    FakeDevice fake( deviceBlock );
    cb->audioDeviceAboutToStart( &fake );

    std::vector<float> l( (size_t)deviceBlock ), r( (size_t)deviceBlock ), inL( (size_t)deviceBlock ), inR( (size_t)deviceBlock );
    float* outs[] = { l.data(), r.data() };
    const float* ins[] = { inL.data(), inR.data() };

    long  delivered = 0, leadingZeros = 0;
    float expected = 1.0f;
    bool  started = false, ok = true;
    for( int c = 0; c < callbacks && ok; ++c )
    {
        cb->audioDeviceIOCallbackWithContext( ins, 2, outs, 2, deviceBlock, {} );
        for( int i = 0; i < deviceBlock; ++i )
        {
            if( !started && l[(size_t)i] == 0.0f ) { ++leadingZeros; continue; }
            started = true;
            if( l[(size_t)i] != expected || r[(size_t)i] != expected )
            {
                std::printf( "  block %d: mismatch at callback %d sample %d: got %g want %g\n", deviceBlock, c, i, l[(size_t)i], expected );
                ok = false;
                break;
            }
            expected += 1.0f;
        }
        delivered += deviceBlock;
    }
    std::printf( "block %4d: %s, engine ran %ld samples for %ld delivered (latency %ld)\n", deviceBlock, ok ? "continuous" : "BROKEN", ramp.processed, delivered, leadingZeros );
    // Real time: the engine produced what was played (after the start-up silence) and is at
    // most one block ahead of it.
    long consumed = delivered - leadingZeros;
    return ok && ramp.processed >= consumed && ramp.processed - consumed < engineBlock;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    AudIO::MemoryPool<float>::get().initialize( pool, 64, engineBlock );
    bool all = true;
    for( int bs : { 480, 960, 128, 256, 512, 1024, 64, 300, 1000 } )
        all = run( bs, 2000 ) && all;
    std::printf( all ? "ALL OK\n" : "FAILURES\n" );
    return all ? 0 : 1;
}
