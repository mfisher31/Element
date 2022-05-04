/*
    This file is part of Element
    Copyright (C) 2022  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <clap/clap.h>

#include "engine/CLAPPluginFormat.h"

namespace Element {

//=============================================================================
class CLAPModule final
{
public:
    CLAPModule() {}
    CLAPModule (const char* dso_path)
        : m_path (dso_path)
    {
    }

    ~CLAPModule()
    {
    }

    bool is_open() const noexcept
    {
        return entry != nullptr;
    }

    const std::string& path() const noexcept { return m_path; }

    bool open (const std::string& dso_path)
    {
        m_path = dso_path;
        return is_open();
    }

    bool close()
    {
        m_path.clear();
        if (entry)
        {
            if (entry->deinit)
                entry->deinit();
        }
        entry = nullptr;
        return true;
    }

    const clap_plugin_factory_t* factory (const char* factory_id) const noexcept {
        return (const clap_plugin_factory_t*)(entry != nullptr && entry->get_factory != nullptr ?
            entry->get_factory (factory_id) : nullptr);
    }

private:
    const clap_plugin_entry_t* entry = nullptr;
    std::string m_path;
};

//=============================================================================
// class CLAPAudioPluginInstance : public AudioPluginInstance
// {
// public:

// };

//=============================================================================
class CLAPPluginFormat::Modules
{
public:
    Modules() {}
    ~Modules()
    {
        for (auto* mod : mods)
        {
            mod->close();
            delete mod;
        }
        mods.clear();
    }

    CLAPModule* find (const std::string& path) const noexcept
    {
        for (const auto& mod : mods)
            if (! mod->path().empty() && mod->path() == path)
                return mod;
        return nullptr;
    }

    CLAPModule* find_or_create (const std::string& path)
    {
        if (auto* mod = find (path))
            return mod;
        if (! File::createFileWithoutCheckingPath (path).existsAsFile())
            return nullptr;
        auto newmod = std::make_unique<CLAPModule>();
        if (newmod->open (path)) {
            mods.push_back (newmod.release());
            return mods.back();
        }
        return nullptr;
    }

private:
    std::vector<CLAPModule*> mods;
};

//=============================================================================
CLAPPluginFormat::CLAPPluginFormat()
{
    modules.reset (new Modules());
}

CLAPPluginFormat::~CLAPPluginFormat()
{
    modules.reset();
}

void CLAPPluginFormat::findAllTypesForFile (OwnedArray<PluginDescription>& results, const String& fileOrIdentifier)
{
    ignoreUnused (results);
    if (! fileMightContainThisPluginType (fileOrIdentifier))
        return;
    auto* module = modules->find_or_create (fileOrIdentifier.toStdString());
    if (module == nullptr)
        return;
    auto factory = module->factory (CLAP_PLUGIN_FACTORY_ID);
    if (factory == nullptr)
        return;
    for (uint32_t i = 0; i < factory->get_plugin_count (factory); ++i) {
        if (auto desc = factory->get_plugin_descriptor (factory, i)) {
            
        }  
    }
}

bool CLAPPluginFormat::fileMightContainThisPluginType (const String& fileOrIdentifier)
{
    auto f = File::createFileWithoutCheckingPath (fileOrIdentifier);

#if JUCE_MAC || JUCE_IOS
    return f.existsAsFile() && f.hasFileExtension (".dylib");
#elif JUCE_WINDOWS
    return f.existsAsFile() && f.hasFileExtension (".dll");
#elif JUCE_LINUX || JUCE_BSD || JUCE_ANDROID
    return f.existsAsFile() && f.hasFileExtension (".so");
#endif

    return false;
}

String CLAPPluginFormat::getNameOfPluginFromIdentifier (const String& fileOrIdentifier)
{
    return fileOrIdentifier;
}

bool CLAPPluginFormat::pluginNeedsRescanning (const PluginDescription& desc)
{
    return File (desc.fileOrIdentifier).getLastModificationTime() != desc.lastFileModTime;
}

bool CLAPPluginFormat::doesPluginStillExist (const PluginDescription& desc)
{
    return File::createFileWithoutCheckingPath (desc.fileOrIdentifier).exists();
}

void CLAPPluginFormat::recursiveFileSearch (StringArray& results, const File& dir, const bool recursive)
{
    for (const auto& iter : RangedDirectoryIterator (dir, false, "*", File::findFilesAndDirectories))
    {
        auto f = iter.getFile();
        bool isPlugin = false;

        if (fileMightContainThisPluginType (f.getFullPathName()))
        {
            isPlugin = true;
            results.add (f.getFullPathName());
        }

        if (recursive && (! isPlugin) && f.isDirectory())
            recursiveFileSearch (results, f, true);
    }
}

StringArray CLAPPluginFormat::searchPathsForPlugins (const FileSearchPath& directoriesToSearch,
                                                     bool recursive,
                                                     bool allowAsync)
{
    ignoreUnused (recursive, allowAsync);
    StringArray results;
    for (int j = 0; j < directoriesToSearch.getNumPaths(); ++j)
        recursiveFileSearch (results, directoriesToSearch[j], false);
    return results;
}

FileSearchPath CLAPPluginFormat::getDefaultLocationsToSearch()
{
#if JUCE_MAC
    return FileSearchPath ("~/Library/Audio/Plug-Ins/CLAP;/Library/Audio/Plug-Ins/CLAP");
#elif JUCE_LINUX || JUCE_BSD || JUCE_ANDROID
    return { SystemStats::getEnvironmentVariable ("CLAP_PATH",
                                                  "/usr/lib/clap;/usr/local/lib/clap;~/.clap")
                 .replace (":", ";") };
#elif JUCE_WINDOWS
    FileSearchPath paths;
    // TODO: real paths. see clap/entry.h
    paths.addIfNotAlreadyThere ("CommonFilesFolder\\CLAP");
    paths.addIfNotAlreadyThere ("LocalAppData\\Programs\\Common\\CLAP");
    paths.removeRedundantPaths();
    return paths;
#endif
}

void CLAPPluginFormat::createPluginInstance (const PluginDescription& desc, double initialSampleRate, int initialBufferSize, PluginCreationCallback callback)
{
    ignoreUnused (desc, initialSampleRate, initialBufferSize, callback);
}

} // namespace Element
