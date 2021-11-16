
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

#include "Application.h"
#include "Backend.h"
#include "Globals.h"
#include "Settings.h"
#include "Utils.h"
#include "Version.h"
#include "controllers/AppController.h"
#include "controllers/SessionController.h"
#include "session/PluginManager.h"

namespace Element {
Application::Application() {}
Application::~Application() {}

const String Application::getApplicationName() { return Util::appName(); }
const String Application::getApplicationVersion() { return ProjectInfo::versionString; }
bool Application::moreThanOneInstanceAllowed() { return true; }

void Application::initialise (const String& commandLine)
{
    backend = std::make_unique<JuceBackend> (this);
    if (maybeLaunchSlave (commandLine))
        return;

    if (sendCommandLineToPreexistingInstance())
    {
        quit();
        return;
    }

    initializeModulePath();
    printCopyNotice();
    launchApplication();
}

void Application::actionListenerCallback (const String& message)
{
    if (message == "finishedLaunching")
        finishLaunching();
}

void Application::shutdown()
{
    if (! backend || ! controller)
        return;

    slaves.clearQuick (true);

    {
        auto& world = backend->globals();
        auto engine (world.getAudioEngine());
        auto& plugins (world.getPluginManager());
        auto& settings (world.getSettings());
        auto& midi (world.getMidiEngine());
        auto* props = settings.getUserSettings();
        plugins.setPropertiesFile (nullptr); // must be done before Settings is deleted

        controller->saveSettings();
        controller->deactivate();

        plugins.saveUserPlugins (settings);
        midi.writeSettings (settings);

        if (auto el = world.getDeviceManager().createStateXml())
            props->setValue ("devices", el.get());
        if (auto keymappings = world.getCommandManager().getKeyMappings()->createXml (true))
            props->setValue ("keymappings", keymappings.get());

        engine = nullptr;
        controller = nullptr;
        Logger::setCurrentLogger (nullptr);
        world.setEngine (nullptr);
    };

    backend.reset();
}

void Application::systemRequestedQuit()
{
    if (! controller)
    {
        Application::quit();
        return;
    }

#if defined(EL_PRO)
    auto* sc = controller->findChild<SessionController>();
    auto& world = backend->globals();
    if (world.getSettings().askToSaveSession())
    {
        // - 0 if the third button was pressed ('cancel')
        // - 1 if the first button was pressed ('yes')
        // - 2 if the middle button was pressed ('no')
        const int res = ! sc->hasSessionChanged() ? 2
                                                  : AlertWindow::showYesNoCancelBox (AlertWindow::NoIcon, "Save Session", "This session may have changes. Would you like to save before exiting?");

        if (res == 1)
            sc->saveSession();
        if (res != 0)
            Application::quit();
    }
    else
    {
        if (sc->getSessionFile().existsAsFile())
        {
            sc->saveSession (false, false, false);
        }
        else
        {
            if (AlertWindow::showOkCancelBox (AlertWindow::NoIcon, "Save Session", "This session has not been saved to disk yet.\nWould you like to before exiting?", "Yes", "No"))
            {
                sc->saveSession();
            }
        }

        Application::quit();
    }

#else // lite and solo
    auto* gc = controller->findChild<GraphController>();
    if (world->getSettings().askToSaveSession())
    {
        // - 0 if the third button was pressed ('cancel')
        // - 1 if the first button was pressed ('yes')
        // - 2 if the middle button was pressed ('no')
        const int res = ! gc->hasGraphChanged() ? 2
                                                : AlertWindow::showYesNoCancelBox (AlertWindow::NoIcon, "Save Graph", "This graph may have changes. Would you like to save before exiting?");
        if (res == 1)
            gc->saveGraph (false);

        if (res != 0)
            Application::quit();
    }
    else
    {
        gc->saveGraph (false);
        Application::quit();
    }
#endif
}

void Application::anotherInstanceStarted (const String& commandLine)
{
    if (! controller)
        return;

#if EL_PRO
    if (auto* sc = controller->findChild<SessionController>())
    {
        const auto path = commandLine.unquoted().trim();
        if (File::isAbsolutePath (path))
        {
            const File file (path);
            if (file.hasFileExtension ("els"))
                sc->openFile (file);
            else if (file.hasFileExtension ("elg"))
                sc->importGraph (file);
        }
    }
#else
    if (auto* gc = controller->findChild<GraphController>())
    {
        const auto path = commandLine.unquoted().trim();
        if (File::isAbsolutePath (path))
        {
            const File file (path);
            if (file.hasFileExtension ("elg"))
                gc->openGraph (file);
        }
    }
#endif
}

void Application::resumed()
{
#if JUCE_WINDOWS
    auto& devices (backend->globals().getDeviceManager());
    devices.restartLastAudioDevice();
#endif
}

void Application::finishLaunching()
{
    if (nullptr != controller || backend->has_launched())
        return;
    controller.reset (new AppController (backend->globals()));
    auto& world = backend->globals();
    if (world.getSettings().scanForPluginsOnStartup())
        world.getPluginManager().scanAudioPlugins();

    auto modsdir = File::getCurrentWorkingDirectory().getChildFile ("build/modules");

    controller->run();

#if EL_PRO
    if (auto* sc = controller->findChild<SessionController>())
    {
        const auto path = world.cli.commandLine.unquoted().trim();
        if (File::isAbsolutePath (path))
        {
            const File file (path);
            if (file.hasFileExtension ("els"))
                sc->openFile (File (path));
        }
    }
#endif
}

void Application::printCopyNotice()
{
    String appName = Util::appName();
    appName << " v" << getApplicationVersion() << " (GPL v3)";
    Logger::writeToLog (appName);
    Logger::writeToLog (String ("Copyright (c) 2017-%YEAR% Kushview, LLC.  All rights reserved.\n")
                            .replace ("%YEAR%", String (Time::getCurrentTime().getYear())));
}

bool Application::maybeLaunchSlave (const String& commandLine)
{
    auto* world = &backend->globals();
    slaves.clearQuick (true);
    slaves.add (world->getPluginManager().createAudioPluginScannerSlave());
    StringArray processIds = { EL_PLUGIN_SCANNER_PROCESS_ID };
    for (auto* slave : slaves)
    {
        for (const auto& pid : processIds)
        {
            if (slave->initialiseFromCommandLine (commandLine, pid))
            {
#if JUCE_MAC
                Process::setDockIconVisible (false);
#endif
                juce::shutdownJuce_GUI();
                return true;
            }
        }
    }

    return false;
}

void Application::launchApplication()
{
    if (backend->has_launched())
        return;
    backend->initialize();
}

void Application::initializeModulePath() {}

} // namespace Element
