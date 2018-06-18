
#include "controllers/AppController.h"
#include "controllers/EngineController.h"
#include "controllers/GuiController.h"
#include "controllers/GraphController.h"
#include "controllers/SessionController.h"
#include "engine/GraphProcessor.h"
#include "engine/SubGraphProcessor.h"
#include "gui/UnlockForm.h"
#include "gui/GuiCommon.h"
#include "session/UnlockStatus.h"
#include "Commands.h"
#include "Globals.h"
#include "Messages.h"
#include "Settings.h"
#include "Version.h"

namespace Element {

Globals& AppController::Child::getWorld()
{
    auto* app = dynamic_cast<AppController*> (getRoot());
    jassert(app);
    return app->getWorld();
}

AppController::AppController (Globals& g)
    : world (g)
{
    lastExportedGraph = DataPath::defaultGraphDir();
    
    addChild (new GuiController (g, *this));
    addChild (new EngineController ());
    addChild (new SessionController ());
    g.getCommandManager().registerAllCommandsForTarget (this);
    g.getCommandManager().setFirstCommandTarget (this);
}

AppController::~AppController() { }

void AppController::activate()
{
    const auto recentList = DataPath::applicationDataDir().getChildFile ("RecentFiles.txt");
    if (recentList.existsAsFile())
    {
        FileInputStream stream (recentList);
        recentFiles.restoreFromString (stream.readEntireStreamAsString());
    }

    Controller::activate();
}

void AppController::deactivate()
{
    const auto recentList = DataPath::applicationDataDir().getChildFile ("RecentFiles.txt");
    if (! recentList.existsAsFile())
        recentList.create();
    if (recentList.exists())
        recentList.replaceWithText (recentFiles.toString(), false, false);
    
    Controller::deactivate();
}

void AppController::run()
{
    auto* sessCtl = findChild<SessionController>();
    assert(sessCtl != nullptr);
    sessCtl->setChangesFrozen (true);

    activate();
    
    auto session = getWorld().getSession();

    if (auto* gui = findChild<GuiController>())
        gui->run();
    
    if (auto* sc = findChild<SessionController>())
    {
        if (world.getSettings().openLastUsedSession())
        {
            const auto lastSession = getWorld().getSettings().getUserSettings()->getValue ("lastSession");
            if (File::isAbsolutePath (lastSession))
                sc->openFile (File (lastSession));
        }
        else
        {
            sc->openDefaultSession();
        }
    }
    
    if (auto* gui = findChild<GuiController>())
    {
        const Node graph (session->getCurrentGraph());
        gui->stabilizeContent();
        if (graph.isValid())
            gui->showPluginWindowsFor (graph);
    }
    
    sessCtl->resetChanges();
    sessCtl->setChangesFrozen (false);
}

void AppController::handleMessage (const Message& msg)
{
	auto* ec   = findChild<EngineController>();
    auto* gui  = findChild<GuiController>();
    auto* sess = findChild<SessionController>();
	jassert(ec && gui && sess);

    bool handled = true;

    if (const auto* lpm = dynamic_cast<const LoadPluginMessage*> (&msg))
    {
        ec->addPlugin (lpm->description, lpm->verified, lpm->relativeX, lpm->relativeY);
    }
    else if (const auto* rnm = dynamic_cast<const RemoveNodeMessage*> (&msg))
    {
        if (rnm->nodes.isEmpty())
        {
            if (rnm->node.isValid())
                ec->removeNode (rnm->node);
            else if (rnm->node.getParentGraph().isRootGraph())
                ec->removeNode (rnm->nodeId);
            else
                handled = false;
        }
        else
        {
            NodeArray graphs;
            for (const auto& node : rnm->nodes)
            {
                if (node.isRootGraph())
                    graphs.add (node);
                else if (node.isValid())
                    ec->removeNode (node);
            }

            for (const auto& graph : graphs) 
            {
                // noop
            }
        }
    }
    else if (const auto* acm = dynamic_cast<const AddConnectionMessage*> (&msg))
    {
        if (acm->useChannels())
        {
            jassertfalse;
            // ec->connectChannels (acm->sourceNode, acm->sourceChannel, acm->destNode, acm->destChannel);
        }
        else
        {
            if (!acm->target.isValid() || acm->target.isRootGraph())
                ec->addConnection (acm->sourceNode, acm->sourcePort, acm->destNode, acm->destPort);
            else if (acm->target.isGraph())
                ec->addConnection (acm->sourceNode, acm->sourcePort, acm->destNode, acm->destPort, acm->target);
            else
                handled = false;
        }
    }
    else if (const auto* rcm = dynamic_cast<const RemoveConnectionMessage*> (&msg))
    {
        if (rcm->useChannels())
		{
            jassertfalse; // channels not yet supported
        }
        else
        {
            if (! rcm->target.isValid() || rcm->target.isRootGraph())
                ec->removeConnection (rcm->sourceNode, rcm->sourcePort, rcm->destNode, rcm->destPort);
            else if (rcm->target.isGraph())
                ec->removeConnection (rcm->sourceNode, rcm->sourcePort, rcm->destNode, rcm->destPort, rcm->target);
            else
                handled = false;
        }
    }
    else if (const auto* dnm = dynamic_cast<const DuplicateNodeMessage*> (&msg))
    {
        Node node = dnm->node;
        ValueTree parent (node.getValueTree().getParent());
        if (parent.hasType (Tags::nodes))
            parent = parent.getParent();
        jassert (parent.hasType (Tags::node));

        const Node graph (parent, false);
        node.savePluginState();
        Node newNode (node.getValueTree().createCopy(), false);
        
        if (newNode.isValid() && graph.isValid())
        {
            newNode.getValueTree().removeProperty (Tags::id, 0);
            ConnectionBuilder dummy;
            ec->addNode (newNode, graph, dummy);
        }
    }
    else if (const auto* dnm2 = dynamic_cast<const DisconnectNodeMessage*> (&msg))
    {
        ec->disconnectNode (dnm2->node, dnm2->inputs, dnm2->outputs);
    }
    else if (const auto* aps = dynamic_cast<const AddPresetMessage*> (&msg))
    {
        const DataPath path;
        String name = aps->name;
        bool canceled = false;
        
        if (name.isEmpty ())
        {
            AlertWindow alert ("Add Preset", "Enter preset name", AlertWindow::NoIcon, 0);
            alert.addTextEditor ("name", aps->node.getName());
            alert.addButton ("Save", 1, KeyPress (KeyPress::returnKey));
            alert.addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
            canceled = 0 == alert.runModalLoop();
            name = alert.getTextEditorContents ("name");
        }
        
        if (! canceled)
        {
             if (! aps->node.savePresetTo (path, name))
                AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Preset", "Could not save preset");
             if (auto* cc = gui->getContentComponent())
                cc->stabilize (true);
        }
    }
    else if (const auto* anm = dynamic_cast<const AddNodeMessage*> (&msg))
    {
        if (anm->node.isGraph() && !getWorld().getUnlockStatus().isFullVersion())
        {
            Alert::showProductLockedAlert ("Nested graphs are not supported without a license");
        }

        else if (anm->target.isValid ())
            ec->addNode (anm->node, anm->target, anm->builder);
        else
            ec->addNode (anm->node);
    }
    else if (const auto* apm = dynamic_cast<const AddPluginMessage*> (&msg))
    {
        const auto graph (apm->graph);
        const auto desc (apm->description);
        
        if (desc.fileOrIdentifier == "element.graph" && !getWorld().getUnlockStatus().isFullVersion())
        {
            Alert::showProductLockedAlert ("Nested graphs are not supported without a license");
        }
        else if (graph.isGraph())
        {
            ec->addPlugin (graph, desc, apm->builder);
        }
        else
        {
            handled = false;
        }
    }
    else if (const auto* cbm = dynamic_cast<const ChangeBusesLayout*> (&msg))
    {
        ec->changeBusesLayout (cbm->node, cbm->layout);
    }
    else if (const auto* osm = dynamic_cast<const OpenSessionMessage*> (&msg))
    {
        sess->openFile (osm->file);
        recentFiles.addFile (osm->file);
    }
    else if (const auto* mdm = dynamic_cast<const AddMidiDeviceMessage*> (&msg))
    {
        ec->addMidiDeviceNode (mdm->device, mdm->inputDevice);
    }
    else
    {
        handled = false;
    }
    
    if (! handled)
    {
        DBG("[EL] AppController: unhandled Message received");
    }
}

ApplicationCommandTarget* AppController::getNextCommandTarget()
{
    return findChild<GuiController>();
}

void AppController::getAllCommands (Array<CommandID>& cids)
{
    cids.addArray ({
        Commands::mediaNew,
        Commands::mediaOpen,
        Commands::mediaSave,
        Commands::mediaSaveAs,
        
        Commands::signIn,
        Commands::signOut,
        
        Commands::sessionNew,
        Commands::sessionSave,
        Commands::sessionSaveAs,
        Commands::sessionOpen,
        Commands::sessionAddGraph,
        Commands::sessionDuplicateGraph,
        Commands::sessionDeleteGraph,
        Commands::sessionInsertPlugin,
        
        Commands::importGraph,
        Commands::exportGraph,
        Commands::panic,
        
        Commands::checkNewerVersion,
        
        Commands::transportPlay
    });
    cids.addArray({ Commands::copy, Commands::paste });
}

void AppController::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
{
    findChild<GuiController>()->getCommandInfo (commandID, result);
}

bool AppController::perform (const InvocationInfo& info)
{
    auto& status (world.getUnlockStatus());
	auto& settings(getGlobals().getSettings());
    bool res = true;
    switch (info.commandID)
    {
        case Commands::sessionOpen:
        {
            FileChooser chooser ("Open Session", lastSavedFile, "*.els", true, false);
            if (chooser.browseForFileToOpen())
            {
                findChild<SessionController>()->openFile (chooser.getResult());
                recentFiles.addFile (chooser.getResult());
            }

        } break;
        case Commands::sessionNew:
            findChild<SessionController>()->newSession();
            break;
        case Commands::sessionSave:
            findChild<SessionController>()->saveSession (false);
            break;
        case Commands::sessionSaveAs:
            findChild<SessionController>()->saveSession (true);
            break;
        case Commands::sessionClose:
            findChild<SessionController>()->closeSession();
            break;
        case Commands::sessionAddGraph:
            findChild<EngineController>()->addGraph();
            break;
        case Commands::sessionDuplicateGraph:
            findChild<EngineController>()->duplicateGraph();
            break;
        case Commands::sessionDeleteGraph:
            findChild<EngineController>()->removeGraph();
            break;
        
        case Commands::transportPlay:
            getWorld().getAudioEngine()->togglePlayPause();
            break;
            
        case Commands::importGraph:
        {
            if (world.getUnlockStatus().isFullVersion())
            {
                FileChooser chooser ("Import Graph", lastExportedGraph, "*.elg");
                if (chooser.browseForFileToOpen())
                    findChild<SessionController>()->importGraph (chooser.getResult());
            }
            else
            {
                Alert::showProductLockedAlert();
            }
            
        } break;
            
        case Commands::exportGraph:
        {
            auto session = getWorld().getSession();
            auto node = session->getCurrentGraph();
            node.savePluginState();
            
            if (!lastExportedGraph.isDirectory())
                lastExportedGraph = lastExportedGraph.getParentDirectory();
            if (lastExportedGraph.isDirectory())
            {
                lastExportedGraph = lastExportedGraph.getChildFile(node.getName()).withFileExtension ("elg");
                lastExportedGraph = lastExportedGraph.getNonexistentSibling();
            }
            if (world.getUnlockStatus().isFullVersion())
            {
                FileChooser chooser ("Export Graph", lastExportedGraph, "*.elg");
                if (chooser.browseForFileToSave (true))
                    findChild<SessionController>()->exportGraph (node, chooser.getResult());
                if (auto* gui = findChild<GuiController>())
                    gui->stabilizeContent();
            }
            else
            {
                Alert::showProductLockedAlert();
            }
        } break;

        case Commands::panic:
        {
            auto e = getWorld().getAudioEngine();
            for (int c = 1; c <= 16; ++c)
            {
                auto msg = MidiMessage::allNotesOff (c);
                msg.setTimeStamp (Time::getMillisecondCounterHiRes());
                e->addMidiMessage (msg);
                msg = MidiMessage::allSoundOff(c);
                msg.setTimeStamp (Time::getMillisecondCounterHiRes());
                e->addMidiMessage (msg);
            }
        }  break;
            
        case Commands::mediaNew:
        case Commands::mediaSave:
        case Commands::mediaSaveAs:
            break;
        
        case Commands::signIn:
        {
            auto* form = new UnlockForm (getWorld(), "Enter your license key.",
                                         false, false, true, true);
            DialogWindow::LaunchOptions opts;
            opts.content.setOwned (form);
            opts.resizable = false;
            opts.dialogTitle = "License Manager";
            opts.runModal();
        } break;
        
        case Commands::signOut:
        {
            if (status.isUnlocked())
            {
                auto* props = settings.getUserSettings();
                props->removeValue("L");
                props->save();
                props->reload();
                status.load();
            }
        } break;
        
        case Commands::checkNewerVersion:
            CurrentVersion::checkAfterDelay (20, true);
            break;
        
        default: res = false; break;
    }
    return res;
}

}