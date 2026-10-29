/*******************************************************************************
 The block below describes the properties of this module, and is read by
 the Projucer to automatically generate project code that uses it.
 For details about the syntax and how to create or use a module, see the
 JUCE Module Format.txt file.

 BEGIN_JUCE_MODULE_DECLARATION

  ID:                 kick_desktop
  vendor:             Linux desktop port contributors
  version:            0.1.0
  name:               Desktop Linux integration
  description:        Window, keyboard and on-screen control panel for the desktop Linux build
  website:
  license:            AGPLv3

  dependencies:       kick_ui_controls, audio_engine, juce_gui_basics, juce_audio_utils
  linuxLibs:          X11

 END_JUCE_MODULE_DECLARATION

*******************************************************************************/

/*
 * This file is part of the Linux desktop port of Kick.
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

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <kick_ui_controls/kick_ui_controls.h>
#include <audio_engine/audio_engine.h>
#include <array>
#include <map>

#include "contracts/dcl/dcl_contracts.h"
#include "contracts/utils/keyboard/keyboard_detector.h"
#include "input_events/input_events.h"

#include "source/desktop_input.h"
#include "source/desktop_hardware_state.h"
#include "source/desktop_keymap.h"
#include "source/desktop_control_panel.h"
#include "source/desktop_audio_settings.h"
#include "source/desktop_window.h"
