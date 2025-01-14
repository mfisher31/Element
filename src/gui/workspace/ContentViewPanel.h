/*
    This file is part of Element
    Copyright (C) 2019  Kushview, LLC.  All rights reserved.

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

#include "gui/ContentComponent.h"
#include "gui/workspace/WorkspacePanel.h"
#include "gui/PluginManagerComponent.h"
#include "gui/views/ControllerDevicesView.h"
#include "gui/views/ControllerMapsView.h"
#include "gui/views/GraphSettingsView.h"
#include "gui/views/KeymapEditorView.h"
#include "gui/views/NodeMidiContentView.h"
#include "gui/views/SessionTreeContentView.h"
#include "gui/views/SessionSettingsView.h"
#include "gui/views/LuaConsoleView.h"
#include "gui/ViewHelpers.h"
#include "gui/views/NodeChannelStripView.h"
#include "gui/views/NodeEditorContentView.h"
#include "gui/views/ScriptEditorView.h"

namespace element {

template <class ViewType>
class ContentViewPanel : public WorkspacePanel
{
public:
    ContentViewPanel() : WorkspacePanel()
    {
        addAndMakeVisible (view);
    }

    ~ContentViewPanel() {}

    void initializeView (ServiceManager& app) override { view.initializeView (app); }
    void didBecomeActive() override { view.didBecomeActive(); }
    void stabilizeContent() override { view.stabilizeContent(); }

    void resized() override
    {
        view.setBounds (getLocalBounds());
    }

    const ContentView& getView() const noexcept { return view; }
    ContentView& getView() noexcept { return view; }

protected:
    ViewType view;
};

class ControllerDevicesPanel : public ContentViewPanel<ControllerDevicesView>
{
public:
    ControllerDevicesPanel() { setName ("Controllers"); }
    ~ControllerDevicesPanel() = default;
};

class ControllerMapsPanel : public ContentViewPanel<ControllerMapsView>
{
public:
    ControllerMapsPanel() { setName ("Maps"); }
    ~ControllerMapsPanel() = default;
};

class GraphSettingsPanel : public ContentViewPanel<GraphSettingsView>
{
public:
    GraphSettingsPanel() { setName ("Graph Settings"); }
    ~GraphSettingsPanel() = default;
};

class KeymapEditorPanel : public ContentViewPanel<KeymapEditorView>
{
public:
    KeymapEditorPanel() { setName ("Keymappings"); }
    ~KeymapEditorPanel() = default;
};

class NodeChannelStripPanel : public ContentViewPanel<NodeChannelStripView>
{
public:
    NodeChannelStripPanel() { setName ("Strip"); }
    ~NodeChannelStripPanel() = default;
};

class NodeEditorPanel : public ContentViewPanel<NodeEditorContentView>
{
public:
    NodeEditorPanel() { setName ("Node"); }
    ~NodeEditorPanel() = default;
};

class NodeMidiPanel : public ContentViewPanel<NodeMidiContentView>
{
public:
    NodeMidiPanel() { setName ("MIDI"); }
    ~NodeMidiPanel() = default;
};

class SessionPanel : public ContentViewPanel<SessionTreeContentView>
{
public:
    SessionPanel() { setName ("Session"); }
    ~SessionPanel() = default;
};

class SessionSettingsPanel : public ContentViewPanel<SessionSettingsView>
{
public:
    SessionSettingsPanel() { setName ("Session Settings"); }
    ~SessionSettingsPanel() = default;
};

class LuaConsolePanel : public ContentViewPanel<LuaConsoleView>
{
public:
    LuaConsolePanel() { setName ("Console"); }
    ~LuaConsolePanel() = default;
};

class PluginManagerPanel : public ContentViewPanel<PluginManagerContentView>
{
public:
    PluginManagerPanel() { setName ("Plugin Manager"); }
    ~PluginManagerPanel() = default;
};

class CodeEditorPanel : public WorkspacePanel
{
public:
    explicit CodeEditorPanel (ScriptEditorView* editor = nullptr)
        : view (editor)
    {
        setName ("Code Editor");
    }

    ~CodeEditorPanel() = default;

    void setView (ScriptEditorView* newView)
    {
        if (view)
            view->willBeRemoved();

        view.reset (newView);
        addAndMakeVisible (view.get());
        resized();
        if (auto* cc = ViewHelpers::findContentComponent (this))
        {
            view->initializeView (cc->getServices());
            view->willBecomeActive();
            view->didBecomeActive();
            view->stabilizeContent();
        }
    }

    void resized() override
    {
        if (view)
            view->setBounds (getLocalBounds());
    }

    void initializeView (ServiceManager& app) override
    {
        if (view)
            view->initializeView (app);
    }
    void didBecomeActive() override
    {
        if (view)
            view->didBecomeActive();
    }
    void stabilizeContent() override
    {
        if (view)
            view->stabilizeContent();
    }

protected:
    std::unique_ptr<ScriptEditorView> view;
};

} // namespace element
