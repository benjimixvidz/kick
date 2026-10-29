/*
 * Copyright (c) 2022 MUSIC Tribe
 *
 * This file is part of Kick.
 *
 * Kick is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Kick is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Kick. If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

//common
#define MT_SOURCE_SRCDIR "."

#if defined( KICK_DESKTOP )
namespace
{
    // Folder name used under the XDG base directories.
    constexpr const char* desktopAppFolder = "kick-desktop";

    juce::File xdgDir( const char* envVar, const char* fallbackUnderHome )
    {
        auto value = juce::SystemStats::getEnvironmentVariable( envVar, {} );
        if( juce::File::isAbsolutePath( value ) )
            return juce::File( value ).getChildFile( desktopAppFolder );
        return juce::File::getSpecialLocation( juce::File::userHomeDirectory ).getChildFile( fallbackUnderHome ).getChildFile( desktopAppFolder );
    }

    // Where assets/, demos/ and templates/ live, first match wins:
    // $KICK_DATA_DIR, <prefix>/share/kick-desktop next to the installed binary, the source tree.
    juce::File findContentDir()
    {
        auto hasAssets = []( const juce::File& d ) { return d.getChildFile( "assets" ).isDirectory(); };

        if( auto env = juce::SystemStats::getEnvironmentVariable( "KICK_DATA_DIR", {} ); juce::File::isAbsolutePath( env ) && hasAssets( juce::File( env ) ) )
            return juce::File( env );

        auto exeDir    = juce::File::getSpecialLocation( juce::File::currentExecutableFile ).getParentDirectory();
        auto installed = exeDir.getParentDirectory().getChildFile( "share" ).getChildFile( desktopAppFolder );
        if( hasAssets( installed ) )
            return installed;
        if( hasAssets( exeDir ) )
            return exeDir;

    #if defined( KICK_SOURCE_DIR )
        if( hasAssets( juce::File( KICK_SOURCE_DIR ) ) )
            return juce::File( KICK_SOURCE_DIR );
    #endif
        return exeDir;
    }

    struct DesktopDirs
    {
        juce::File content{ findContentDir() };
        juce::File data{ xdgDir( "XDG_DATA_HOME", ".local/share" ) };
        juce::File state{ xdgDir( "XDG_STATE_HOME", ".local/state" ) };
        juce::File config{ xdgDir( "XDG_CONFIG_HOME", ".config" ) };

        DesktopDirs()
        {
            for( auto sub : { data.getChildFile( "projects" ), data.getChildFile( "sounds" ), state.getChildFile( "logs" ), config.getChildFile( "prefs" ) } )
                sub.createDirectory();
        }
    };
}
#endif

juce::File getAppDirs( DirType T )
{
    auto seprator = juce::File::getSeparatorString();
#if defined( _WIN32 ) || defined( _WIN64 )
    auto rootP = juce::File::getSpecialLocation( juce::File::currentApplicationFile ).getParentDirectory().getFullPathName() + seprator;
    if( T == MTAssets )
        return juce::File( rootP + juce::String( MT_SOURCE_SRCDIR ) + seprator + "assets" );
    else if( T == MTSamples )
        return juce::File( rootP + juce::String( MT_SOURCE_SRCDIR ) + seprator + "samples" );
    else if( T == MTDemos )
        return juce::File( rootP + juce::String( MT_SOURCE_SRCDIR ) + seprator + "demos" );
    else if( T == MTTemp )
        return juce::File( rootP + juce::String( MT_SOURCE_SRCDIR ) + seprator + "templates" );
    else if( T == MTProjects )
        return juce::File( rootP + juce::String( MT_SOURCE_SRCDIR ) + seprator + "projects" );
    else if( T == MTLog )
        return juce::File( rootP + juce::String( MT_SOURCE_SRCDIR ) + seprator + "logs" + seprator );
    else if( T == MTPrefs )
        return juce::File( rootP + juce::String( MT_SOURCE_SRCDIR ) + seprator + "prefs" + seprator );
#elif defined( __APPLE_CPP__ ) || defined( __APPLE_CC__ ) || defined( JUCE_MAC )
    if( T == MTAssets )
        return juce::File( "./assets" );
    else if( T == MTSamples )
        return juce::File( "./data/samplesKick/" );
    else if( T == MTDemos )
        return juce::File( "./demos" );
    else if( T == MTTemp )
        return juce::File( "./templates" );
    else if( T == MTProjects )
        return juce::File( "./projects" );
    else if( T == MTLog )
        return juce::File( "./logs/" );
    else if( T == MTPrefs )
        return juce::File( "./prefs" );
#elif defined( KICK_DESKTOP )
    // Read-only content (assets, demos, templates) comes from the install or source tree,
    // everything the user creates goes to the XDG base directories. Resolved once.
    static const DesktopDirs dirs;
    if( T == MTAssets )
        return dirs.content.getChildFile( "assets" );
    else if( T == MTSamples )
        return dirs.data.getChildFile( "sounds" );
    else if( T == MTDemos )
        return dirs.content.getChildFile( "demos" );
    else if( T == MTTemp )
        return dirs.content.getChildFile( "templates" );
    else if( T == MTProjects )
        return dirs.data.getChildFile( "projects" );
    else if( T == MTLog )
        return dirs.state.getChildFile( "logs" );
    else if( T == MTPrefs )
        return dirs.config.getChildFile( "prefs" );
#elif defined( LINUX ) || defined( __linux__ )
    if( T == MTAssets )
        return juce::File( "/app/assets" );
    else if( T == MTSamples )
        return juce::File( "/data/sounds" );
    else if( T == MTDemos )
        return juce::File( "/app/demos/" );
    else if( T == MTTemp )
        return juce::File( "/app/templates/" );
    else if( T == MTProjects )
        return juce::File( "/home/Kick/projects/" );
    else if( T == MTLog )
        return juce::File( "/home/Kick/logs/" );
    else if( T == MTPrefs )
        return juce::File( "/home/Kick/prefs" );
#endif

    return juce::File::getSpecialLocation( juce::File::tempDirectory );
}

juce::File getDefaultPath()
{
    juce::File rootP;
    auto       seprator = juce::File::getSeparatorString();

#if defined( _WIN32 ) || defined( _WIN64 )
    rootP = juce::File::getSpecialLocation( juce::File::currentApplicationFile ).getParentDirectory().getFullPathName() + seprator;

#elif defined( __APPLE_CPP__ ) || defined( __APPLE_CC__ ) || defined( JUCE_MAC )
    rootP = "./app/";

#elif defined( KICK_DESKTOP )
    rootP = getAppDirs( MTProjects ).getParentDirectory();

#elif defined( LINUX ) || defined( __linux__ )
    rootP = "./app/";
#endif
    return rootP;
}
