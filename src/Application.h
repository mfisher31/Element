
#pragma once

#include <element/juce.hpp>
#include <juce_gui_basics/juce_gui_basics.h>
#include <kv_core/kv_core.h>

namespace Element {

class AppController;
class Globals;

class JUCE_API Application : public juce::JUCEApplication,
                             private juce::AsyncUpdater
{
public:
    using String = juce::String;
    Application();
    virtual ~Application();

    const String getApplicationName()    override;
    const String getApplicationVersion() override;
    bool moreThanOneInstanceAllowed()    override;

    void initialise (const String& commandLine ) override;
    
    void shutdown() override;

    void systemRequestedQuit() override;

    void anotherInstanceStarted (const String& commandLine) override;

    void resumed() override;

    void finishLaunching();
    
    Globals* getWorld() const noexcept { return world.get(); }
    AppController* getAppController() const noexcept { return controller.get(); }

private:
    String launchCommandLine;
    
    std::unique_ptr<Globals>            world;
    std::unique_ptr<AppController>      controller;
    juce::OwnedArray<kv::ChildProcessSlave>   slaves;

    void printCopyNotice();
    bool maybeLaunchSlave (const String& commandLine);
    void launchApplication();
    void initializeModulePath();

    void handleAsyncUpdate() override;
};
}