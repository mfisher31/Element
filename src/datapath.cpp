/*
    This file is part of Element
    Copyright (c) 2019 Kushview, LLC.  All rights reserved.

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
*/

#include "datapath.hpp"
#include "session/node.hpp"

namespace element {
namespace DataPathHelpers {
    StringArray getSubDirs()
    {
        auto dirs = StringArray ({ "Controllers", "Graphs", "Presets", "Scripts" });
#ifndef EL_SOLO
        dirs.add ("Sessions");
#endif
        return dirs;
    }

    void initializeUserLibrary (const File& path)
    {
        for (const auto& d : getSubDirs())
        {
            const auto subdir = path.getChildFile (d);
            if (subdir.existsAsFile())
                subdir.deleteFile();
            subdir.createDirectory();
        }
    }
} // namespace DataPathHelpers

DataPath::DataPath()
{
    root = defaultUserDataPath();
    DataPathHelpers::initializeUserLibrary (root);
}

DataPath::~DataPath() {}

const File DataPath::applicationDataDir()
{
#if JUCE_MAC
    return File::getSpecialLocation (File::userApplicationDataDirectory)
        .getChildFile ("Application Support/Element");
#else
    return File::getSpecialLocation (File::userApplicationDataDirectory)
        .getChildFile ("Element");
#endif
}

const File DataPath::defaultUserDataPath()
{
    return File::getSpecialLocation (File::userMusicDirectory)
        .getChildFile ("Element");
}

const File DataPath::defaultSettingsFile()
{
#if JUCE_DEBUG
    return applicationDataDir().getChildFile ("ElementDebug.conf");
#else
    return applicationDataDir().getChildFile ("Element.conf");
#endif
}

const File DataPath::defaultLocation() { return defaultUserDataPath(); }

const File DataPath::defaultGlobalMidiProgramsDir()
{
    return defaultUserDataPath().getChildFile ("Cache/MIDI/Programs");
}

const File DataPath::defaultScriptsDir() { return defaultUserDataPath().getChildFile ("Scripts"); }
const File DataPath::defaultSessionDir() { return defaultUserDataPath().getChildFile ("Sessions"); }
const File DataPath::defaultGraphDir() { return defaultUserDataPath().getChildFile ("Graphs"); }
const File DataPath::defaultControllersDir() { return defaultUserDataPath().getChildFile ("Controllers"); }

File DataPath::createNewPresetFile (const Node& node, const String& name) const
{
    String path = "Presets/";
    if (name.isNotEmpty())
        path << name;
    else
        path << String (node.getName().isNotEmpty() ? node.getName() : "New Preset");
    path << ".elpreset";
    return getRootDir().getChildFile (path).getNonexistentSibling();
}

void DataPath::findPresetsFor (const String& format, const String& identifier, NodeArray& nodes) const
{
    const auto presetsDir = getRootDir().getChildFile ("Presets");
    if (! presetsDir.exists() || ! presetsDir.isDirectory())
        return;

    DirectoryIterator iter (presetsDir, true, EL_PRESET_FILE_EXTENSIONS);
    while (iter.next())
    {
        Node node (Node::parse (iter.getFile()));
        if (node.isValid() && node.getFileOrIdentifier() == identifier && node.getFormat() == format)
        {
            nodes.add (node);
        }
    }
}

void DataPath::findPresetFiles (StringArray& results) const
{
    const auto presetsDir = getRootDir().getChildFile ("Presets");
    if (! presetsDir.exists() || ! presetsDir.isDirectory())
        return;
    DirectoryIterator iter (presetsDir, true, EL_PRESET_FILE_EXTENSIONS);
    while (iter.next())
        results.add (iter.getFile().getFullPathName());
}

const File DataPath::workspacesDir()
{
    const auto dir = applicationDataDir().getChildFile ("Workspaces");
    if (dir.existsAsFile())
        dir.deleteFile();
    if (! dir.exists())
        dir.createDirectory();
    return dir;
}

const File DataPath::installDir()
{
    File dir;

#if JUCE_WINDOWS
    const auto installDir = WindowsRegistry::getValue (
                                "HKEY_LOCAL_MACHINE\\Software\\Kushview\\Element\\InstallDir", "")
                                .unquoted();
    if (File::isAbsolutePath (installDir))
        dir = File (installDir);
#elif JUCE_MAC
    dir = File ("/Applications");
#endif

    return dir;
}
} // namespace element
