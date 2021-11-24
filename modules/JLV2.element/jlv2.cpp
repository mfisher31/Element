/** 
    This file is part of libelement.
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

#include <element/juce/plugin.hpp>
#define JLV2_PLUGINHOST_LV2 1
#include "../../libs/jlv2/modules/jlv2_host/jlv2_host.cpp"

struct JLV2 final {
    JLV2()
    {
        memset (&format, 0, sizeof (elJuceAudioPluginFormat));
        format.handle = this;
        format.create = &JLV2::create_format;
        extensions.insert ({ EL_JUCE__AudioPluginFormat, &format });
    }

    ~JLV2() {}

    static juce::AudioPluginFormat* create_format (elHandle)
    {
        return new jlv2::LV2PluginFormat();
    }

    elJuceAudioPluginFormat format;

    const void* extension (const char* key) const noexcept
    {
        auto it = extensions.find (key);
        return it != extensions.end() ? it->second : nullptr;
    }

private:
    std::map<std::string, const void*> extensions;
};

static elHandle jlv2_new() { return new JLV2(); }

static const void* jlv2_extension (elHandle handle, const char* ID)
{
    return ((JLV2*) handle)->extension (ID);
}

static void jlv2_destroy (elHandle handle)
{
    delete static_cast<JLV2*> (handle);
}

const elDescriptor* element_descriptor()
{
    const static elDescriptor M = {
        .ID = "org.lvtk.JLV2",
        .create = jlv2_new,
        .extension = jlv2_extension,
        .load = nullptr,
        .unload = nullptr,
        .destroy = jlv2_destroy
    };
    return &M;
}
