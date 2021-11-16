
// #ifndef EL_APPLICATION_H
// #define EL_APPLICATION_H

// #include <element/module.h>

// #ifdef __cplusplus
// extern "C" {
// #endif

// EL_EXPORT int element_main (int argc, const char** argv);

// #ifdef __cplusplus
// }
// #endif

// #endif

#pragma once

#include <element/context.hpp>
#include <element/juce.hpp>
#include <juce_gui_basics/juce_gui_basics.h>
#include <kv_core/kv_core.h>

namespace Element {

class AppController;
class JuceBackend;

class JUCE_API Application : public juce::JUCEApplication,
                             public juce::ActionListener
{
public:
    using String = juce::String;
    Application();
    virtual ~Application();

    const String getApplicationName()    override;
    const String getApplicationVersion() override;
    bool moreThanOneInstanceAllowed()    override;

    void initialise (const String& commandLine ) override;
    
    void actionListenerCallback (const String& message) override;
    
    void shutdown() override;

    void systemRequestedQuit() override;

    void anotherInstanceStarted (const String& commandLine) override;

    void resumed() override;

    void finishLaunching();
    
private:
    String launchCommandLine;
    
    std::unique_ptr<JuceBackend>        backend; 
    std::unique_ptr<AppController>      controller;
    juce::OwnedArray<kv::ChildProcessSlave>   slaves;

    void printCopyNotice();
    bool maybeLaunchSlave (const String& commandLine);
    void launchApplication();
    void initializeModulePath();
};
}