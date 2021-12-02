#include <element/context.hpp>
#include <element/juce/plugin.hpp>
#include <lua.hpp>

#include "Application.h"
#include "Backend.h"
#include "Globals.h"
#include "session/PluginManager.h"
#include "controllers/AppController.h"

#define EL_MODULE__UI EL_PREFIX "UI"

extern "C" {
extern int luaopen_el_Rectangle (lua_State*);
}

namespace Element {
    extern void initializeWorld (Globals&);
    extern void shutdownWorld (Globals&, AppController&);
}

struct UI final {
    UI () {
        // app.reset (new Element::Application());
        world.reset (new Element::Globals());
        Element::initializeWorld (*world);
        controller.reset (new Element::AppController (*world));
    }

    ~UI() {
        app.reset();
    }

    bool initializeApp() {
        if (auto* a = dynamic_cast<JUCEApplicationBase*> (app.get()))
            return a->initialiseApp();
        return false;
    }

    int shutdownApp() {
        return app != nullptr ? app->shutdownApp() : -1;
    }

    Element::Globals& getWorld() { return *world; }
    Element::AppController& getApp() { return *controller; }

    std::unique_ptr<Element::Application> app;
    std::unique_ptr<Element::Globals> world;
    std::unique_ptr<Element::AppController> controller;
};

static elHandle ui_create()
{
    juce::initialiseJuce_GUI();
    return new UI();
};

static void ui_load (elHandle handle, elFeatures features)
{
    auto ui = (UI*) handle;
    // if (! ui->initializeApp()) {
    //     ui->shutdownApp();
    //     std::clog << "couldn't init\n";
    //     return;
    // }

    EL_FEATURES_FOREACH (features, f) {
        if (strcmp (f->ID, EL_JUCE__AudioPluginFormat) == 0) {
            auto ext = (const elJuceAudioPluginFormat*)f->data;
            if (auto fmt = std::unique_ptr<juce::AudioPluginFormat> (ext->create (ext->handle))) {
                auto& plugins = ui->world->getPluginManager();
                plugins.addFormat (fmt.release());
            }
        }
    }
}

static void ui_unload (elHandle handle)
{
    std::clog << __PRETTY_FUNCTION__ << std::endl;
    auto ui = (UI*) handle;
    ui->controller.reset();
    ui->world.reset();
}

static void ui_destroy (elHandle handle)
{
    std::clog << __PRETTY_FUNCTION__ << std::endl;
    delete (UI*) handle;
    juce::shutdownJuce_GUI();
}

static int ui_main (elHandle handle, int argc, const char* argv[])
{
    using namespace juce;
    auto ui = (UI*) handle;

    JUCE_AUTORELEASEPOOL
    {
        ui->controller->run();
// #if JUCE_MAC
//         juce::initialiseNSApplication();
// #endif

// #if JUCE_IOS && JUCE_MODULE_AVAILABLE_juce_gui_basics
//         return juce_iOSMain (argc, argv, iOSCustomDelegate);
// #else
//         jassert (nullptr != ui->app);

        JUCE_TRY
        {
            // loop until a quit message is received..

            while (MessageManager::getInstance()->runDispatchLoopUntil(14) && ! ui->controller->shouldQuit()) {}

            // MessageManager::getInstance()->runDispatchLoop();
            std::clog << "event loop finished\n";
        }
        JUCE_CATCH_EXCEPTION
        Element::shutdownWorld (ui->getWorld(), ui->getApp());
        ui->controller->resetQuitFlag();
        return 0;


//         JUCE_TRY
//         {
//             ui->app->shutdown();
//         }
//         JUCE_CATCH_EXCEPTION

//         auto ret = ui->app->getApplicationReturnValue();

//         return ret;
// #endif
    }
}

static const void* ui_extension (elHandle handle, const char* name) {
    if (strcmp (name, "el.Main") == 0) {
        static const elMain mainextension = {
            .main = ui_main
        };
        return (const void*) &mainextension;
    }
    return nullptr;
}

EL_EXPORT
const elDescriptor* element_descriptor()
{
    static const luaL_Reg packages[] = {
        { "el.Rectangle",   luaopen_el_Rectangle },
        { nullptr, nullptr }
    };

    static const elDescriptor D = {
        .ID = EL_MODULE__UI,
        .create = ui_create,
        .extension = ui_extension,
        .load = ui_load,
        .unload = ui_unload,
        .destroy = ui_destroy
    };

    return &D;
}
