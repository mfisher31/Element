#include <element/context.hpp>
#include <element/juce/plugin.hpp>
#include <lua.hpp>

#include "Application.h"
#include "Backend.h"
#include "Globals.h"
#include "controllers/AppController.h"
#include "session/PluginManager.h"

#define EL_MODULE__UI EL_PREFIX "UI"

class TestContent : public juce::Component {
public:
    TestContent()
    {
        embed.reset (new XEmbedComponent (false, false));
        // addAndMakeVisible (embed.get());
        setSize (640, 360);
        setOpaque (true);
    }

    void resized()
    {
        embed->setBounds (getLocalBounds().reduced (10));
        // embed->centreWithSize (640, 360);
    }

    void paint (juce::Graphics& g)
    {
        g.fillAll (juce::Colours::black.brighter());
        g.setColour (juce::Colours::white);
        g.drawText ("Hello World", getLocalBounds().toFloat(), juce::Justification::centred, true);
    }

    void setDisplayEnabled (bool enabled)
    {
        if (enabled) {
            addAndMakeVisible (embed.get());
            embed->setVisible (true);
        } else {
            removeAllChildren();
            embed->setVisible (false);
        }
    }

    evgSwapSetup getSwapSetup() const
    {
        evgSwapSetup setup;
        setup.adapter = 0;
        setup.width = 640;
        setup.height = 360;
        setup.nbuffers = 1;
        setup.window.xwindow = embed->getHostWindowID();
        setup.zstencil_format = EVG_ZSTENCIL_24_S8;
        return std::move (setup);
    }

private:
    std::unique_ptr<XEmbedComponent> embed;
};

class TestWindow : public juce::DocumentWindow {
public:
    TestWindow()
        : juce::DocumentWindow ("Test Window", juce::Colours::black,
                                juce::DocumentWindow::allButtons, false)
    {
        content.reset (new TestContent());
        setContentNonOwned (content.get(), true);
        setUsingNativeTitleBar (true);
        setResizable (true, false);
    }

    void closeButtonPressed() override
    {
        quitflag = true;
    }

    bool shouldQuit() const { return quitflag; }
    TestContent* getTestContent() const noexcept { return content.get(); }

private:
    std::unique_ptr<TestContent> content;
    bool quitflag = false;
};

extern "C" {
extern int luaopen_el_Rectangle (lua_State*);
}

namespace Element {
extern void initializeWorld (Globals&);
extern void shutdownWorld (Globals&, AppController&);
} // namespace Element

#define TEST_WINDOW 1
struct UI final {
    UI()
    {
#if TEST_WINDOW
// noop
#else
        app.reset (new Element::Application());
        world.reset (new Element::Globals());
        Element::initializeWorld (*world);
        controller.reset (new Element::AppController (*world));
#endif
    }

    ~UI()
    {
        app.reset();
        window.reset();
    }

    bool initializeApp()
    {
        if (auto* a = dynamic_cast<JUCEApplicationBase*> (app.get()))
            return a->initialiseApp();
        return false;
    }

    int shutdownApp()
    {
        return app != nullptr ? app->shutdownApp() : -1;
    }

    Element::Globals& getWorld() { return *world; }
    Element::AppController& getApp() { return *controller; }
    std::unique_ptr<Element::Application> app;
    std::unique_ptr<Element::Globals> world;
    std::unique_ptr<Element::AppController> controller;
    std::unique_ptr<TestWindow> window;
    element::Context* context;
};

static elHandle ui_create()
{
    juce::initialiseJuce_GUI();
    return new UI();
};

static void ui_load (elHandle handle, elFeatures features)
{
    auto ui = (UI*) handle;

    std::clog << "ui_load\n";
    EL_FEATURES_FOREACH (features, f) {
        std::clog << "check: " << f->ID << std::endl;
        if (strcmp (f->ID, EL_JUCE__AudioPluginFormat) == 0) {
            auto ext = (const elJuceAudioPluginFormat*) f->data;
            if (auto fmt = std::unique_ptr<juce::AudioPluginFormat> (ext->create (ext->handle))) {
                auto& plugins = ui->world->getPluginManager();
                plugins.addFormat (fmt.release());
            }
        } else if (strcmp (f->ID, "el.Context") == 0) {
            std::clog << "[ui] got a element::Context\n";
            ui->context = (element::Context*) f->data;
        }
    }
}

static void ui_unload (elHandle handle)
{
#if ! TEST_WINDOW
    std::clog << __PRETTY_FUNCTION__ << std::endl;
    auto ui = (UI*) handle;
    ui->controller.reset();
    ui->world.reset();
#endif
}

static void ui_destroy (elHandle handle)
{
#if ! TEST_WINDOW
    std::clog << __PRETTY_FUNCTION__ << std::endl;
    delete (UI*) handle;
#endif
    juce::shutdownJuce_GUI();
}

static int ui_main (elHandle handle, int argc, const char* argv[])
{
    using namespace juce;
    auto ui = (UI*) handle;

    if (ui->context == nullptr)
        return -1;

    JUCE_AUTORELEASEPOOL
    {
#if ! TEST_WINDOW
        ui->controller->run();
#else
        ui->window.reset (new TestWindow());
        ui->window->centreWithSize (ui->window->getWidth(), ui->window->getHeight());
        ui->window->addToDesktop();
        ui->window->setVisible (true);
        auto setup = ui->window->getTestContent()->getSwapSetup();
        if (nullptr != ui->context->test_create_video_display (&setup))
            ui->window->getTestContent()->setDisplayEnabled (true);

#endif
        JUCE_TRY
        {
            while (MessageManager::getInstance()->runDispatchLoopUntil (14) && ! ui->window->shouldQuit())
                ;
        }
        JUCE_CATCH_EXCEPTION

#if ! TEST_WINDOW
        Element::shutdownWorld (ui->getWorld(), ui->getApp());
        ui->controller->resetQuitFlag();
#else
        ui->window->getTestContent()->setDisplayEnabled (false);
        ui->context->test_create_video_display (nullptr);
        ui->window->removeFromDesktop();
        ui->window.reset();
#endif
        return 0;
    }
}

static const void* ui_extension (elHandle handle, const char* name)
{
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
        { "el.Rectangle", luaopen_el_Rectangle },
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
