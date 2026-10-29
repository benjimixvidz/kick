/*
 * This file is part of the Linux desktop port of Kick.
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

// The three Xlib calls used here, declared directly: the X11 headers define macros such as
// KeyPress and None that clash with JUCE in this translation unit.
extern "C"
{
    struct _XDisplay;
    _XDisplay*    XOpenDisplay( const char* name );
    int           XCloseDisplay( _XDisplay* display );
    unsigned long XkbKeycodeToKeysym( _XDisplay* display, unsigned char keycode, int group, int level );
}

namespace desktop
{
    namespace
    {
        // Physical position of every printable key the panel map uses, given as the character
        // it carries on a US QWERTY keyboard and its X11 keycode (evdev code + 8).
        const std::map<int, unsigned char> qwertyPositions = {
            { '1', 10 }, { '2', 11 }, { '3', 12 }, { '4', 13 }, { '5', 14 }, { '6', 15 }, { '7', 16 },
            { '8', 17 }, { '9', 18 }, { '0', 19 }, { '-', 20 }, { '=', 21 },
            { 'Q', 24 }, { 'W', 25 }, { 'E', 26 }, { 'R', 27 }, { 'T', 28 }, { 'Y', 29 }, { 'U', 30 },
            { 'I', 31 }, { 'O', 32 }, { 'P', 33 }, { '[', 34 }, { ']', 35 },
            { 'A', 38 }, { 'S', 39 }, { 'D', 40 }, { 'F', 41 }, { 'G', 42 }, { 'H', 43 }, { 'J', 44 },
            { 'K', 45 }, { 'L', 46 },
            { 'Z', 52 }, { 'X', 53 }, { 'C', 54 }, { 'V', 55 }, { 'B', 56 }, { 'N', 57 }, { 'M', 58 },
            { ',', 59 }, { '.', 60 },
        };

        // JUCE passes a key code straight to XKeysymToKeycode, so the unshifted keysym of the
        // physical key is the key code to watch. Letters use JUCE's upper-case convention.
        int keysymToKeyCode( unsigned long keysym, int fallback )
        {
            if( keysym == 0 || keysym >= 0x10000 ) // NoSymbol, or a Unicode keysym JUCE cannot express
                return fallback;
            if( keysym >= 'a' && keysym <= 'z' )
                return (int)( keysym - 'a' + 'A' );
            return (int)keysym;
        }
    }

    KeyMap buildKeyMap()
    {
        const auto& qwertyMap = contracts::getKeypressDetector().getKeyMap();

        auto* display = XOpenDisplay( nullptr );
        if( display == nullptr )
            return qwertyMap;

        KeyMap map;
        for( const auto& [keyCode, info] : qwertyMap )
        {
            auto pos = qwertyPositions.find( keyCode );
            if( pos == qwertyPositions.end() )
            {
                map.emplace( keyCode, info );
                continue;
            }

            auto layoutKey = keysymToKeyCode( XkbKeycodeToKeysym( display, pos->second, 0, 0 ), keyCode );
            if( !map.emplace( layoutKey, info ).second )
                map.emplace( keyCode, info ); // two positions gave the same symbol: keep the original key
        }

        XCloseDisplay( display );
        return map;
    }

    namespace
    {
        juce::String keyLabel( int keyCode )
        {
            using KP = juce::KeyPress;
            static const std::map<int, const char*> names = {
                { KP::spaceKey, "Space" }, { KP::returnKey, "Enter" }, { KP::escapeKey, "Esc" },
                { KP::tabKey, "Tab" }, { KP::deleteKey, "Del" }, { KP::insertKey, "Ins" },
                { KP::pageUpKey, "PgUp" }, { KP::pageDownKey, "PgDn" }, { KP::upKey, "Up" },
                { KP::downKey, "Down" }, { KP::leftKey, "Left" }, { KP::rightKey, "Right" },
            };
            if( auto it = names.find( keyCode ); it != names.end() )
                return it->second;
            if( keyCode >= KP::F1Key && keyCode <= KP::F12Key )
                return "F" + juce::String( keyCode - KP::F1Key + 1 );
            if( keyCode > ' ' && keyCode < 0x100 )
                return juce::String::charToString( (juce::juce_wchar)keyCode ).toUpperCase();
            return {};
        }

        juce::String findLabel( const KeyMap& map, CPktMgr::enumPacketType type, int id )
        {
            // Several keys can share a control (e.g. F11 and the media Stop key): show one that
            // has a printable name.
            for( const auto& [keyCode, info] : map )
                if( info.id == id && info.pktType == type )
                    if( auto label = keyLabel( keyCode ); label.isNotEmpty() )
                        return label;
            return {};
        }
    }

    juce::String keyLabelForButton( const KeyMap& map, int buttonId ) { return findLabel( map, CPktMgr::EN_PKT_TYPE_BUTTON, buttonId ); }
    juce::String keyLabelForPad( const KeyMap& map, int pad ) { return findLabel( map, CPktMgr::EN_PKT_TYPE_PAD, pad ); }
}
