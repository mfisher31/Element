
#include <element/context.hpp>
#include <element/juce/plugin.hpp>
#include <lua.hpp>

#include "Application.h"
#include "Backend.h"
#include "Globals.h"
#include "Logs.h"
#include "Settings.h"
#include "controllers/AppController.h"
#include "engine/InternalFormat.h"
#include "session/PluginManager.h"

namespace Element {

class Startup : public ActionBroadcaster
{
public:
    Startup (Globals& w, const bool useThread = false, const bool splash = false)
        : world (w),
          usingThread (useThread),
          showSplash (splash),
          isFirstRun (false)
    {
    }

    ~Startup() {}

    void launchApplication()
    {
        Settings& settings (world.getSettings());
        isFirstRun = ! settings.getUserSettings()->getFile().existsAsFile();
        DataPath path;
        ignoreUnused (path);

        updateSettingsIfNeeded();

        DeviceManager& devices (world.getDeviceManager());
        auto* props = settings.getUserSettings();
        if (auto dxml = props->getXmlValue ("devices"))
        {
            devices.initialise (DeviceManager::maxAudioChannels,
                                DeviceManager::maxAudioChannels,
                                dxml.get(),
                                true,
                                "default",
                                nullptr);
        }
        else
        {
            devices.initialiseWithDefaultDevices (
                DeviceManager::maxAudioChannels,
                DeviceManager::maxAudioChannels);
        }

        run();
    }

private:
    friend class Application;
    Globals& world;
    const bool usingThread;
    const bool showSplash;
    bool isFirstRun;

    void updateSettingsIfNeeded()
    {
        Settings& settings (world.getSettings());
        ignoreUnused (settings);
    }

    void run()
    {
        Settings& settings (world.getSettings());
        AudioEnginePtr engine = new AudioEngine (world);
        engine->applySettings (settings);
        world.setEngine (engine); // this will also instantiate the session

        setupLogging();
        setupPlugins();
        setupKeyMappings();
        setupAudioEngine();
        setupMidiEngine();
        setupScripting();

        sendActionMessage ("finishedLaunching");
    }

    void setupAudioEngine() {}

    void setupMidiEngine()
    {
        auto& midi = world.getMidiEngine();
        midi.applySettings (world.getSettings());
    }

    void setupKeyMappings()
    {
        auto* const props = world.getSettings().getUserSettings();
        auto* const keymp = world.getCommandManager().getKeyMappings();
        if (props && keymp)
        {
            std::unique_ptr<XmlElement> xml;
            xml = props->getXmlValue ("keymappings");
            if (xml != nullptr)
                world.getCommandManager().getKeyMappings()->restoreFromXml (
                    *xml);
            xml = nullptr;
        }
    }

    void setupPlugins()
    {
        auto& settings (world.getSettings());
        auto& plugins (world.getPluginManager());
        auto engine (world.getAudioEngine());

        plugins.addDefaultFormats();
        plugins.addFormat (new InternalFormat (*engine, world.getMidiEngine()));
        plugins.addFormat (new ElementAudioPluginFormat (world));
        plugins.restoreUserPlugins (settings);
        plugins.setPropertiesFile (settings.getUserSettings());
        plugins.scanInternalPlugins();
        plugins.searchUnverifiedPlugins();
    }

    void setupScripting()
    {
        auto& scripts = world.getScriptingEngine();
        ignoreUnused (scripts);
    }

    void setupLogging() { Logger::setCurrentLogger (&world.getLog()); }
};

void initializeWorld (Globals& g)
{
    auto startup = std::make_unique<Startup> (g, false, false);
    startup->launchApplication();
    return;
}

void shutdownWorld (Globals& g, AppController& a) {
    auto* const world = &g;
    auto* const controller = &a;

    auto engine (world->getAudioEngine());
    auto& plugins (world->getPluginManager());
    auto& settings (world->getSettings());
    auto& midi (world->getMidiEngine());
    auto* props = settings.getUserSettings();
    plugins.setPropertiesFile (nullptr); // must be done before Settings is deleted

    controller->saveSettings();
    controller->deactivate();

    plugins.saveUserPlugins (settings);
    midi.writeSettings (settings);

    if (auto el = world->getDeviceManager().createStateXml())
        props->setValue ("devices", el.get());
    if (auto keymappings = world->getCommandManager().getKeyMappings()->createXml (true))
        props->setValue ("keymappings", keymappings.get());

    Logger::setCurrentLogger (nullptr);
    world->setEngine (nullptr);
}

} // namespace Element