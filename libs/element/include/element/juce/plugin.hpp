/** 
    This file is part of Element
    Copyright (C) 2016-2021  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
**/

#include <element/juce.hpp>
#include <element/plugin.h>

#define EL_JUCE__AudioPluginFormat "juce.AudioPluginFormat"

/** This is the data type returned for the juce.AudioPluginFormat extension */
typedef struct {
    /** Handle to internal data */
    elHandle handle;
    /** Factory function. */
    juce::AudioPluginFormat* (*create)(elHandle handle);
} elJuceAudioPluginFormat;