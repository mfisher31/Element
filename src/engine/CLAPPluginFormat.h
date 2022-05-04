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

#pragma once

#include "ElementApp.h"

namespace Element {

class CLAPPluginFormat : public AudioPluginFormat
{
public:
    CLAPPluginFormat();
    ~CLAPPluginFormat();

    String getName() const override { return "CLAP"; }
    void findAllTypesForFile (OwnedArray<PluginDescription>& results, const String& fileOrIdentifier) override;
    bool fileMightContainThisPluginType (const String& fileOrIdentifier) override;
    String getNameOfPluginFromIdentifier (const String& fileOrIdentifier) override;
    bool pluginNeedsRescanning (const PluginDescription&) override;
    bool doesPluginStillExist (const PluginDescription&) override;
    bool canScanForPlugins() const override { return true; }
    bool isTrivialToScan() const override { return false; }

    StringArray searchPathsForPlugins (const FileSearchPath& directoriesToSearch,
                                       bool recursive,
                                       bool allowPluginsWhichRequireAsynchronousInstantiation = false) override;
    FileSearchPath getDefaultLocationsToSearch() override;

protected:
    void createPluginInstance (const PluginDescription&, double initialSampleRate,
                               int initialBufferSize, PluginCreationCallback) override;
    bool requiresUnblockedMessageThreadDuringCreation (const PluginDescription&) const override { return false; }

private:
    class Modules;
    std::unique_ptr<Modules> modules;
    void recursiveFileSearch (StringArray& results, const File& dir, const bool recursive);
};

}
