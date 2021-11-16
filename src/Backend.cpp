
#include <element/context.hpp>
#include <element/juce/plugin.hpp>

#include "controllers/AppController.h"
#include "engine/InternalFormat.h"
#include "session/PluginManager.h"
#include "Globals.h"
#include "Logs.h"
#include "Settings.h"
#include "Backend.h"

namespace Element {

class Startup : public ActionBroadcaster {
public:
    Startup(Globals& w, const bool useThread = false, const bool splash = false)
            : world(w),
              usingThread(useThread),
              showSplash(splash),
              isFirstRun(false)
    { }

    ~Startup() {}

    void launchApplication()
    {
        Settings& settings(world.getSettings());
        isFirstRun = !settings.getUserSettings()->getFile().existsAsFile();
        DataPath path;
        ignoreUnused(path);

        updateSettingsIfNeeded();

        DeviceManager& devices(world.getDeviceManager());
        auto* props = settings.getUserSettings();
        if (auto dxml = props->getXmlValue("devices")) {
            devices.initialise(DeviceManager::maxAudioChannels,
                               DeviceManager::maxAudioChannels, dxml.get(),
                               true, "default", nullptr);
        } else {
            devices.initialiseWithDefaultDevices(
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
        Settings& settings(world.getSettings());
        ignoreUnused(settings);
    }

    void run()
    {
        Settings& settings(world.getSettings());
        AudioEnginePtr engine = new AudioEngine(world);
        engine->applySettings(settings);
        world.setEngine(engine); // this will also instantiate the session

        setupLogging();
        setupPlugins();
        setupKeyMappings();
        setupAudioEngine();
        setupMidiEngine();
        setupScripting();

        sendActionMessage("finishedLaunching");
    }

    void setupAudioEngine() {}

    void setupMidiEngine()
    {
        auto& midi = world.getMidiEngine();
        midi.applySettings(world.getSettings());
    }

    void setupKeyMappings()
    {
        auto* const props = world.getSettings().getUserSettings();
        auto* const keymp = world.getCommandManager().getKeyMappings();
        if (props && keymp) {
            std::unique_ptr<XmlElement> xml;
            xml = props->getXmlValue("keymappings");
            if (xml != nullptr)
                world.getCommandManager().getKeyMappings()->restoreFromXml(
                        *xml);
            xml = nullptr;
        }
    }

    void setupPlugins()
    {
        auto& settings(world.getSettings());
        auto& plugins(world.getPluginManager());
        auto engine(world.getAudioEngine());

        plugins.addDefaultFormats();
        plugins.addFormat(new InternalFormat(*engine, world.getMidiEngine()));
        plugins.addFormat(new ElementAudioPluginFormat(world));
        plugins.restoreUserPlugins(settings);
        plugins.setPropertiesFile(settings.getUserSettings());
        plugins.scanInternalPlugins();
        plugins.searchUnverifiedPlugins();
    }

    void setupScripting()
    {
        auto& scripts = world.getScriptingEngine();
        ignoreUnused(scripts);
    }

    void setupLogging() { Logger::setCurrentLogger(&world.getLog()); }
};

/** JUCE general support in plugins */
class JUCEFeatureType : public element::FeatureType {
public:
    JUCEFeatureType(Element::PluginManager& _plugins) : plugins(_plugins)
    {
        data.handle = &plugins;
        data.register_audio_plugin_format = register_audio_plugin_format;
        set_details(EL_FEATURE__JuceRegistrar, &data);
    };

private:
    elJuceRegistrar data;
    Element::PluginManager& plugins;

    static bool register_audio_plugin_format(void* handle, void* format)
    {
        if (handle == nullptr || format == nullptr)
            return false;
        auto& plugins = *(Element::PluginManager*)handle;
        plugins.addFormat((juce::AudioPluginFormat*)format);
        return true;
    }
};

JuceBackend::JuceBackend(ActionListener* listener) : app(listener)
{
    world.reset (new Globals());
    std::clog << "world.reset (new Globals());\n";
    add_feature (new JUCEFeatureType (world->getPluginManager()));
}

JuceBackend::~JuceBackend() {}

void JuceBackend::initialize()
{
    startup.reset (new Startup(*world, false, false));
    startup->addActionListener (app);
    startup->launchApplication();

    File path = File::getCurrentWorkingDirectory();
    path = path.getChildFile ("build/modules/JLV2.element");
    load_module (path.getFullPathName().toStdString());
}

Globals& JuceBackend::globals()
{
    return *world;
}

AppController* JuceBackend::finish_and_return_app()
{
    startup.reset();
    launched = true;
    return nullptr;
}

}
