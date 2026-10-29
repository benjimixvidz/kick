# Checks that mt::AudioDevice keeps the engine output continuous and in real time for device
# buffer sizes that are and are not multiples of the engine block.
#
#   cmake -S tests/audio-device-fifo -B build-test -DJUCE_DIR=<JUCE 7.0.12 checkout>
#   cmake --build build-test && build-test/AudioDeviceFifoTest_artefacts/AudioDeviceFifoTest
cmake_minimum_required(VERSION 3.22)
project(AudioDeviceFifoTest VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)

get_filename_component(KICK_REPO "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
add_subdirectory(${JUCE_DIR} juce)
juce_add_modules(${KICK_REPO}/AudIO/AudIO ${KICK_REPO}/AudIO/AudIO_constants ${KICK_REPO}/AudIO/AudIO_juce)

juce_add_console_app(AudioDeviceFifoTest)
target_sources(AudioDeviceFifoTest PRIVATE main.cpp)
target_compile_definitions(AudioDeviceFifoTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_JACK=0)
target_link_libraries(AudioDeviceFifoTest PRIVATE AudIO_juce juce::juce_audio_devices)
